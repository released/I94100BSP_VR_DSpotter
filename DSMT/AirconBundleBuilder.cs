using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Web.Script.Serialization;
using System.Xml;

internal static class AirconBundleBuilder
{
    private const uint ModelAddress = 0x15000;
    private const uint FlashEnd = 0x80000;
    private const uint AudioAddress = 0x70000;
    private static readonly JavaScriptSerializer Json = new JavaScriptSerializer
    {
        MaxJsonLength = 16 * 1024 * 1024,
        RecursionLimit = 100
    };
    private static readonly UTF8Encoding Utf8 = new UTF8Encoding(false, true);
    private static readonly Dictionary<string, string> Inputs = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    private static readonly StringBuilder Log = new StringBuilder();

    private sealed class Options
    {
        public string Root;
        public string Output;
        public string Firmware;
        public bool Pause;
        public bool Help;
    }

    private sealed class NativeModel
    {
        public byte[] Pack;
        public Dictionary<string, object> Summary;
    }

    private static int Main(string[] args)
    {
        Options options = null;
        int result = 1;
        try
        {
            try { Console.OutputEncoding = new UTF8Encoding(false); } catch (IOException) { }
            options = Parse(args);
            if (options.Help)
            {
                Say("BuildLatestModel.cmd [--output CMDSet\\ReleaseV005] [--firmware path] [--no-pause]");
                Say("Advanced: --root project-directory. No Python, PowerShell or Keil process is started.");
            }
            else
            {
                Say("I94124 model/audio builder 2.0 - no Python required");
                Say("Started: " + DateTimeOffset.Now.ToString("o", CultureInfo.InvariantCulture));
                Build(options);
            }
            result = 0;
        }
        catch (Exception ex)
        {
            Say("ERROR: " + ex.Message);
            Say("Do not program files from a failed build. Correct the input and run again.");
        }
        finally
        {
            if (options != null && !options.Help)
            {
                try
                {
                    string directory = Path.Combine(options.Root, "DSMT", "Test");
                    OrdinaryPath(options.Root, directory);
                    Directory.CreateDirectory(directory);
                    string name = "BundleBuild_" + DateTime.Now.ToString("yyyyMMdd_HHmmss_fff", CultureInfo.InvariantCulture) + ".log";
                    string path = Path.Combine(directory, name);
                    File.WriteAllText(path, Log.ToString(), new UTF8Encoding(true));
                    Console.WriteLine("Log: " + path);
                }
                catch (Exception ex) { Console.WriteLine("Log could not be saved: " + ex.Message); }
            }
            if (options != null && options.Pause && !Console.IsInputRedirected)
            {
                Console.WriteLine("Press any key to close...");
                Console.ReadKey(true);
            }
        }
        return result;
    }

    private static Options Parse(string[] args)
    {
        Options o = new Options();
        o.Root = Path.GetDirectoryName(AppDomain.CurrentDomain.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar));
        for (int i = 0; i < args.Length; ++i)
        {
            string arg = args[i];
            if (arg == "--pause") o.Pause = true;
            else if (arg == "--no-pause") o.Pause = false;
            else if (arg == "--help" || arg == "-h") o.Help = true;
            else if (arg == "--root" || arg == "--output" || arg == "--firmware")
            {
                if (++i == args.Length) throw new ArgumentException("Missing value for " + arg);
                if (arg == "--root") o.Root = args[i];
                else if (arg == "--output") o.Output = args[i];
                else o.Firmware = args[i];
            }
            else throw new ArgumentException("Unknown argument: " + arg);
        }
        o.Root = Path.GetFullPath(o.Root);
        o.Output = FullPath(o.Root, o.Output ?? Path.Combine("CMDSet", "ReleaseV005"));
        RequireInside(Path.Combine(o.Root, "CMDSet"), o.Output);
        OrdinaryPath(o.Root, o.Output);
        return o;
    }

    private static void Build(Options o)
    {
        string manifestPath = Path.Combine(o.Root, "DSMT", "Input", "CommandManifest.json");
        object[] records = ArrayValue(ReadJson(manifestPath)["records"]);
        NativeModel chinese = LoadNative(o.Root, "Chinese(TWN)", "ChineseTWN", "PanasonicAirConZHTW.dsmt", records);
        NativeModel taiwanese = LoadNative(o.Root, "Taiwanese", "Taiwanese", "Taiwanese.dsmt", records);
        byte[] bundle = Bundle(chinese.Pack, taiwanese.Pack);
        if (bundle.Length > AudioAddress - ModelAddress) throw new InvalidDataException("Dual-language models overlap the internal audio region.");
        Dictionary<string, object> audioInfo;
        byte[] audio = BuildAudio(o.Root, out audioInfo);

        string firmwarePath;
        if (o.Firmware != null) firmwarePath = FullPath(o.Root, o.Firmware);
        else
        {
            firmwarePath = Path.Combine(o.Root, "Prj", "KEIL", "obj", "DNN_NuOne_UAC.bin");
            if (!File.Exists(firmwarePath))
            {
                firmwarePath = Path.Combine(o.Root, "CMDSet", "ReleaseV005", "DNN_NuOne_UAC.bin");
                Dictionary<string, object> published = ReadJson(Path.Combine(o.Root, "CMDSet", "ReleaseV005", "ModelManifest.json"));
                Dictionary<string, object> info = Dict(Dict(published["files"])["DNN_NuOne_UAC.bin"]);
                if (Sha(Read(firmwarePath)) != Text(info, "sha256")) throw new InvalidDataException("Published firmware fallback hash mismatch.");
                Say("Keil output absent: using hash-verified compatible published V005 firmware.");
            }
        }
        byte[] firmware = Read(firmwarePath);
        if (firmware.Length == 0 || firmware.Length > ModelAddress) throw new InvalidDataException("Firmware is empty or overlaps the model region.");
        if (!Encoding.ASCII.GetString(firmware).Contains("[APP] release=V005 feedback=full_reply_led"))
            throw new InvalidDataException("V005 firmware with internal audio support is required.");
        Say("Firmware source: " + firmwarePath);

        Dictionary<string, byte[]> payloads = new Dictionary<string, byte[]>(StringComparer.Ordinal);
        payloads.Add("ChineseTWN.pack.bin", chinese.Pack);
        payloads.Add("Taiwanese.pack.bin", taiwanese.Pack);
        payloads.Add("AirconModels.bundle.bin", bundle);
        payloads.Add("DNN_NuOne_UAC.bin", firmware);
        payloads.Add("AirconReplies.bin", audio);
        byte[] dataFlash = new byte[AudioAddress - ModelAddress + (uint)audio.Length];
        for (int i = 0; i < dataFlash.Length; ++i) dataFlash[i] = 0xFF;
        Buffer.BlockCopy(bundle, 0, dataFlash, 0, bundle.Length);
        Buffer.BlockCopy(audio, 0, dataFlash, (int)(AudioAddress - ModelAddress), audio.Length);
        payloads.Add("AirconDataFlash.bin", dataFlash);
        Dictionary<string, object> fileInfo = new Dictionary<string, object>();
        foreach (KeyValuePair<string, byte[]> item in payloads)
            fileInfo.Add(item.Key, D("bytes", item.Value.Length, "sha256", Sha(item.Value)));
        int missing = ArrayValue(chinese.Summary["missing_records"]).Length + ArrayValue(taiwanese.Summary["missing_records"]).Length;
        Dictionary<string, object> manifest = D(
            "format", "ACB1", "format_version", 1, "languages", new object[] { chinese.Summary, taiwanese.Summary },
            "release", "V005", "audio", audioInfo,
            "programming", D("DNN_NuOne_UAC.bin", "0x00000000", "AirconDataFlash.bin", "0x00015000"),
            "component_addresses", D("AirconModels.bundle.bin", "0x00015000", "AirconReplies.bin", "0x00070000"),
            "model_capacity_end_exclusive", Hex(AudioAddress),
            "model_end_exclusive", Hex(ModelAddress + (uint)bundle.Length), "flash_end_exclusive", "0x00080000",
            "files", fileInfo, "verification_limits", new object[] {
                "Acoustic accuracy and real-time processing not board-tested",
                "Actual dual-engine RAM is checked and printed at runtime",
                missing.ToString(CultureInfo.InvariantCulture) + " command text variants remain pending valid native pronunciations; nanoe reading confirmed by user",
                "Existing binary ICP project was not rewritten" });
        Publish(o, payloads, Utf8.GetBytes(Pretty(Json.Serialize(manifest)) + "\n"));
        Say("SUCCESS: AirconModels.bundle.bin = " + bundle.Length + " bytes; model end " + Hex(ModelAddress + (uint)bundle.Length));
        Say("SHA-256: " + Sha(bundle));
        Say("Internal audio: " + audio.Length + " bytes at 0x00070000; 23 NuOne 1.0 clips.");
        Say("ICP DataFlash file: AirconDataFlash.bin (models and internal audio), " + dataFlash.Length + " bytes.");
        Say("Output: " + o.Output);
        Say("Model address: 0x00015000. Firmware address: 0x00000000.");
        Say("DSMT compilation and acoustic testing are separate steps; firmware is copied, not rebuilt.");
    }


    private static byte[] BuildAudio(string root, out Dictionary<string, object> info)
    {
        const string prefix = "AudioRes/ReplyPhrases/";
        const string romRelative = prefix + "Output/AirconReplies_AudioInfoMerge.ROM";
        Dictionary<string, object> seal = ReadJson(FullPath(root, prefix + "AudioBuildManifest.json"));
        Dictionary<string, object> sources = Dict(seal["sources"]);
        if (Number(seal["schema_version"]) != 1 || Number(seal["sample_rate"]) != 16000
            || Number(seal["clips"]) != 23 || Convert.ToDecimal(seal["bits_per_sample"], CultureInfo.InvariantCulture) != 1M)
            throw new InvalidDataException("Audio build manifest settings mismatch.");
        foreach (KeyValuePair<string, object> entry in sources)
        {
            string path = FullPath(root, entry.Key);
            RequireInside(root, path);
            OrdinaryPath(root, path);
            if (Sha(Read(path)) != entry.Value as string)
                throw new InvalidDataException("Audio source changed; rebuild and seal audio: " + entry.Key);
        }
        string[] required = new string[] { prefix + "ReplyProfile.json", prefix + "WavSrc/PhraseSourceManifest.json",
            prefix + "WavSrc/AirconReplies.uid", prefix + "Output/AirconReplies_AudioInfo.h", romRelative };
        foreach (string path in required)
            if (!sources.ContainsKey(path)) throw new InvalidDataException("Missing sealed audio input: " + path);
        Dictionary<string, object> profile = ReadJson(FullPath(root, prefix + "ReplyProfile.json"));
        Dictionary<string, object> wavManifest = ReadJson(FullPath(root, prefix + "WavSrc/PhraseSourceManifest.json"));
        if (Text(wavManifest, "source_profile_sha256") != Sha(Read(FullPath(root, prefix + "ReplyProfile.json"))))
            throw new InvalidDataException("Phrase source profile is stale.");
        object[] phrases = ArrayValue(profile["phrases"]);
        object[] waves = ArrayValue(wavManifest["entries"]);
        if (phrases.Length != 23 || waves.Length != 23 || sources.Count != 28)
            throw new InvalidDataException("Expected 23 sealed audio clips.");
        string header = Utf8.GetString(Read(FullPath(root, prefix + "Output/AirconReplies_AudioInfo.h"))).Replace(" ", "").Replace("\\t", "");
        HashSet<int> mapIds = new HashSet<int>();
        for (int i = 0; i < phrases.Length; ++i)
        {
            Dictionary<string, object> phrase = Dict(phrases[i]);
            Dictionary<string, object> wave = Dict(waves[i]);
            string file = Text(phrase, "file");
            string relative = prefix + "WavSrc/" + file;
            if (Path.GetFileName(file) != file || Number(phrase["index"]) != i || Number(wave["index"]) != i
                || Text(wave, "file") != file || !sources.ContainsKey(relative)
                || Text(wave, "sha256") != Sha(Read(FullPath(root, relative))))
                throw new InvalidDataException("Audio clip order/source mismatch.");
            string symbol = Path.GetFileNameWithoutExtension(file).ToUpperInvariant() + "=" + i.ToString(CultureInfo.InvariantCulture) + ",";
            if (!header.Contains(symbol)) throw new InvalidDataException("Generated audio ID header mismatch: " + symbol);
            foreach (object mapValue in ArrayValue(phrase["map_ids"]))
            {
                int id = Number(mapValue);
                int expected = id >= 1 && id <= 19 ? id + 1 : id >= 116 && id <= 130 ? 21
                    : id >= 201 && id <= 212 ? 22 : id >= 1001 && id <= 1003 ? 0 : -1;
                if (expected != i || !mapIds.Add(id)) throw new InvalidDataException("Audio MapID table mismatch.");
            }
        }
        if (mapIds.Count != 49) throw new InvalidDataException("Audio MapID coverage mismatch.");
        byte[] rom = Read(FullPath(root, romRelative));
        if (rom.Length < 200 || rom.Length + 16 > FlashEnd - AudioAddress
            || ReadU32(rom, 0) != 23 || ReadU32(rom, 4) != 0 || ReadU32(rom, 8) != 16)
            throw new InvalidDataException("Audio ROM header/capacity mismatch.");
        uint cursor = 200;
        for (uint i = 0; i < 23; ++i)
        {
            uint offset = ReadU32(rom, 16 + i * 8);
            uint bytes = ReadU32(rom, 20 + i * 8);
            if (offset != cursor || bytes < 12 || cursor > rom.Length || bytes > rom.Length - cursor)
                throw new InvalidDataException("Audio ROM clip bounds mismatch.");
            uint payload = ReadU32(rom, offset + 4);
            if (ReadU16(rom, offset) != 0x40 || ReadU16(rom, offset + 2) != 16000
                || ReadU32(rom, offset + 8) != 320 || payload == 0 || payload % 40 != 0 || payload > bytes - 12)
                throw new InvalidDataException("Expected NuOne 16 kHz, 1.0 bits/sample audio.");
            cursor += bytes;
        }
        if (cursor != rom.Length) throw new InvalidDataException("Audio ROM trailing bytes mismatch.");
        byte[] image;
        using (MemoryStream stream = new MemoryStream())
        using (BinaryWriter writer = new BinaryWriter(stream))
        {
            writer.Write(0x31524341U); writer.Write(1U); writer.Write((uint)rom.Length); writer.Write(Crc32(rom));
            writer.Write(rom);
            while (stream.Position % 4 != 0) writer.Write((byte)0);
            image = stream.ToArray();
        }
        info = D("format", "ACR1", "address", Hex(AudioAddress), "rom_bytes", rom.Length,
            "rom_sha256", Sha(rom), "clips", 23, "sample_rate", 16000, "bits_per_sample", 1,
            "profile", prefix + "ReplyProfile.json", "profile_sha256", Sha(Read(FullPath(root, prefix + "ReplyProfile.json"))));
        return image;
    }

    private static uint ReadU32(byte[] data, uint offset)
    {
        if (offset > data.Length || data.Length - offset < 4) throw new InvalidDataException("Truncated audio word.");
        int i = (int)offset;
        return (uint)data[i] | ((uint)data[i + 1] << 8) | ((uint)data[i + 2] << 16) | ((uint)data[i + 3] << 24);
    }

    private static uint ReadU16(byte[] data, uint offset)
    {
        if (offset > data.Length || data.Length - offset < 2) throw new InvalidDataException("Truncated audio halfword.");
        return (uint)data[(int)offset] | ((uint)data[(int)offset + 1] << 8);
    }

    private static NativeModel LoadNative(string root, string language, string folder, string filename, object[] records)
    {
        string directory = Path.Combine(root, "DSMT", "Combine", "Working", folder);
        string projectPath = Path.Combine(directory, filename);
        byte[] projectBytes = Read(projectPath);
        Dictionary<string, object> project = JsonObject(projectBytes);
        if (Text(project, "language") != language || Text(Dict(project["platformFormat"]), "nByteAlignment") != "4")
            throw new InvalidDataException("Language/alignment mismatch: " + filename);
        Dictionary<string, Dictionary<string, object>> lookup = new Dictionary<string, Dictionary<string, object>>(StringComparer.Ordinal);
        List<object> ordered = new List<object>();
        foreach (object entry in records)
        {
            Dictionary<string, object> record = Dict(entry);
            if (Text(record, "language") != language) continue;
            string key = Key(Text(record, "group"), Text(record, "text"));
            if (lookup.ContainsKey(key)) throw new InvalidDataException("Duplicate requirement: " + Text(record, "text"));
            lookup.Add(key, record);
            ordered.Add(record);
        }
        if (lookup.Count == 0) throw new InvalidDataException("No requirements for " + language);
        byte[] baseBytes = Read(Path.Combine(directory, "CYBase.mod"));
        List<byte[]> chunks = new List<byte[]> { baseBytes };
        Dictionary<string, object> modelHashes = D("CYBase.mod", Sha(baseBytes));
        HashSet<string> seen = new HashSet<string>(StringComparer.Ordinal);
        HashSet<string> names = new HashSet<string>(StringComparer.Ordinal);
        List<object> summaries = new List<object>();
        object[] groups = ArrayValue(project["groups"]);
        if (groups.Length < 2 || groups.Length > 9) throw new InvalidDataException("Firmware supports 2..9 nonempty groups per language.");
        for (int groupIndex = 0; groupIndex < groups.Length; ++groupIndex)
        {
            Dictionary<string, object> group = Dict(groups[groupIndex]);
            if (group.Count != 1) throw new InvalidDataException("Malformed native group.");
            string name = null;
            foreach (string candidate in group.Keys) name = candidate;
            if (name == null || name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || name.Contains("..") || !names.Add(name))
                throw new InvalidDataException("Invalid or duplicate group name.");
            if (groupIndex == 0 && name != "Group_1_Wake") throw new InvalidDataException("Group_1_Wake must be the first group.");
            object[] words = ArrayValue(group[name]);
            if (words.Length == 0) throw new InvalidDataException("Empty group: " + name);
            Dictionary<string, int> expected = new Dictionary<string, int>(StringComparer.Ordinal);
            HashSet<string> unique = new HashSet<string>(StringComparer.Ordinal);
            foreach (object entry in words)
            {
                Dictionary<string, object> word = Dict(entry);
                string command = Text(word, "command");
                string key = Key(name, command);
                Dictionary<string, object> record;
                if (!lookup.TryGetValue(key, out record)) throw new InvalidDataException("Unknown word/category; update requirements first: " + name + " / " + command);
                int mapId = Number(word["CmdMapID"]);
                if (mapId != Number(record["map_id"]) || Text(word, "phoneme_id").Length == 0 || Text(word, "ipa_encode").Length == 0)
                    throw new InvalidDataException("MapID/native pronunciation mismatch: " + command);
                Count(expected, Key(Text(word, "show_command"), Text(word, "phoneme"), mapId));
                seen.Add(key);
                unique.Add(command);
            }
            string exported = Utf16(Read(Path.Combine(directory, name + ".txt")));
            string[] rawLines = exported.Split(new string[] { "\r\n", "\n", "\r" }, StringSplitOptions.RemoveEmptyEntries);
            List<string> lines = new List<string>();
            foreach (string line in rawLines) if (!String.IsNullOrWhiteSpace(line)) lines.Add(line);
            if (lines.Count == 0) throw new InvalidDataException("Empty exported group: " + name);
            XmlReaderSettings settings = new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null };
            XmlDocument xml = new XmlDocument { XmlResolver = null };
            using (XmlReader reader = XmlReader.Create(new StringReader(lines[0]), settings)) xml.Load(reader);
            if (xml.SelectSingleNode("/setting/language").InnerText != language || xml.SelectSingleNode("/setting/groupName").InnerText != name)
                throw new InvalidDataException("Exported group settings mismatch: " + name);
            Dictionary<string, int> actual = new Dictionary<string, int>(StringComparer.Ordinal);
            for (int i = 1; i < lines.Count; ++i)
            {
                string[] fields = lines[i].Split('\t');
                int pr = System.Array.IndexOf(fields, "-pr");
                int map = System.Array.IndexOf(fields, "-mapid");
                if (pr < 0 || pr + 1 >= fields.Length || map < 0 || map + 1 >= fields.Length)
                    throw new InvalidDataException("Missing -pr/-mapid in exported group: " + name);
                Count(actual, Key(fields[0], fields[pr + 1], Int32.Parse(fields[map + 1], CultureInfo.InvariantCulture)));
            }
            if (!SameCounts(expected, actual)) throw new InvalidDataException("Saved text does not match project; save in DSMT: " + name);
            byte[] model = Read(Path.Combine(directory, name + ".mod"));
            chunks.Add(model);
            modelHashes.Add(name + ".mod", Sha(model));
            summaries.Add(D("group", name, "entries", words.Length, "unique_texts", unique.Count));
        }
        List<object> missing = new List<object>();
        foreach (object entry in ordered)
        {
            Dictionary<string, object> record = Dict(entry);
            if (seen.Contains(Key(Text(record, "group"), Text(record, "text")))) continue;
            if (!Text(record, "text").Contains("nanoe")) throw new InvalidDataException("Non-nanoe command still missing: " + Text(record, "text"));
            missing.Add(record);
        }
        foreach (byte[] chunk in chunks)
            if (chunk.Length < 4 || chunk.Length % 4 != 0) throw new InvalidDataException("Invalid model size/alignment: " + language);
        byte[] native;
        using (MemoryStream stream = new MemoryStream())
        {
            using (BinaryWriter writer = new BinaryWriter(stream))
            {
                writer.Write((uint)chunks.Count);
                foreach (byte[] chunk in chunks) writer.Write((uint)chunk.Length);
                foreach (byte[] chunk in chunks) writer.Write(chunk);
                native = stream.ToArray();
            }
        }
        string packName = Text(project, "projectName") + "_pack.bin";
        RequireInside(directory, Path.Combine(directory, packName));
        if (!Equal(Read(Path.Combine(directory, packName)), native)) throw new InvalidDataException("Native pack components mismatch; save in DSMT: " + packName);
        Say("PASS native " + language + ": " + seen.Count + " texts, " + groups.Length + " groups, " + native.Length + " bytes, pending " + missing.Count);
        return new NativeModel { Pack = native, Summary = D(
            "language", language, "project", Relative(root, projectPath), "project_sha256", Sha(projectBytes),
            "base_release", project["modelReleaseDate"], "groups", summaries.ToArray(), "unique_texts", seen.Count,
            "missing_records", missing.ToArray(), "native_pack_bytes", native.Length, "native_pack_sha256", Sha(native),
            "model_sha256", modelHashes, "human_pronunciation_review_complete", false) };
    }

    private static byte[] Bundle(byte[] chinese, byte[] taiwanese)
    {
        byte[][] packs = new byte[][] { chinese, taiwanese };
        using (MemoryStream stream = new MemoryStream())
        using (BinaryWriter writer = new BinaryWriter(stream))
        {
            uint offset = 48;
            writer.Write(0x31424341U);
            writer.Write(1U);
            writer.Write(offset + (uint)chinese.Length + (uint)taiwanese.Length);
            writer.Write(2U);
            for (int i = 0; i < packs.Length; ++i)
            {
                writer.Write((uint)(i + 1)); writer.Write(offset); writer.Write((uint)packs[i].Length); writer.Write(Crc32(packs[i]));
                offset += (uint)packs[i].Length;
            }
            foreach (byte[] pack in packs) writer.Write(pack);
            return stream.ToArray();
        }
    }

    private static void Publish(Options o, Dictionary<string, byte[]> payloads, byte[] manifest)
    {
        Dictionary<string, byte[]> all = new Dictionary<string, byte[]>(payloads);
        all.Add("ModelManifest.json", manifest);
        CheckInputs();
        Directory.CreateDirectory(o.Output);
        OrdinaryPath(o.Root, o.Output);
        string lockPath = Path.Combine(o.Output, ".bundle-build.lock");
        OrdinaryPath(o.Root, lockPath);
        FileStream buildLock;
        try { buildLock = new FileStream(lockPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None); }
        catch (IOException) { throw new IOException("Another builder is using this output directory, or it is not writable."); }
        try
        {
            using (buildLock)
            {
                string oldManifestPath = Path.Combine(o.Output, "ModelManifest.json");
                Dictionary<string, object> owned = File.Exists(oldManifestPath) ? Dict(ReadJson(oldManifestPath)["files"]) : new Dictionary<string, object>();
                Dictionary<string, byte[]> previous = new Dictionary<string, byte[]>(StringComparer.Ordinal);
                bool changed = false;
                foreach (KeyValuePair<string, byte[]> item in all)
                {
                    string path = Path.Combine(o.Output, item.Key);
                    OrdinaryPath(o.Root, path);
                    byte[] old = File.Exists(path) ? File.ReadAllBytes(path) : null;
                    previous.Add(item.Key, old);
                    if (old != null && !Equal(old, item.Value) && item.Key != "ModelManifest.json")
                    {
                        object info;
                        if (!owned.TryGetValue(item.Key, out info) || Sha(old) != Text(Dict(info), "sha256"))
                            throw new InvalidDataException("Preserve changed output: " + path);
                    }
                    if (old == null || !Equal(old, item.Value)) changed = true;
                }
                CheckInputs();
                if (!changed) { Say("Output already matches validated inputs; no release files changed."); return; }
                BackupOutput(o);
                CheckInputs();
                string stage = Path.Combine(o.Output, ".bundle-stage-" + Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(stage);
                List<string> committed = new List<string>();
                try
                {
                    foreach (KeyValuePair<string, byte[]> item in all) File.WriteAllBytes(Path.Combine(stage, item.Key), item.Value);
                    foreach (KeyValuePair<string, byte[]> item in all)
                    {
                        string target = Path.Combine(o.Output, item.Key);
                        byte[] current = File.Exists(target) ? File.ReadAllBytes(target) : null;
                        if (!Equal(current, previous[item.Key])) throw new IOException("Output changed during build: " + target);
                        string source = Path.Combine(stage, item.Key);
                        if (File.Exists(target)) File.Replace(source, target, null);
                        else File.Move(source, target);
                        committed.Add(item.Key);
                    }
                }
                catch
                {
                    foreach (string name in committed)
                    {
                        string target = Path.Combine(o.Output, name);
                        if (!Equal(File.ReadAllBytes(target), all[name])) throw new IOException("Publication interrupted and output was edited externally. Restore the verified backup before programming.");
                        if (previous[name] == null) File.Delete(target);
                        else File.WriteAllBytes(target, previous[name]);
                    }
                    throw;
                }
                finally
                {
                    RequireInside(o.Output, stage);
                    if (!Path.GetFileName(stage).StartsWith(".bundle-stage-", StringComparison.Ordinal)) throw new IOException("Unexpected stage directory.");
                    foreach (string path in Directory.GetFiles(stage)) File.Delete(path);
                    Directory.Delete(stage);
                }
            }
        }
        finally
        {
            buildLock.Dispose();
            File.Delete(lockPath);
        }
    }

    private static void BackupOutput(Options o)
    {
        Dictionary<string, byte[]> files = new Dictionary<string, byte[]>(StringComparer.Ordinal);
        foreach (string path in Directory.GetFiles(o.Output))
        {
            if (Path.GetFileName(path) == ".bundle-build.lock") continue;
            OrdinaryPath(o.Root, path);
            files.Add(Path.GetFileName(path), File.ReadAllBytes(path));
        }
        if (files.Count == 0) return;
        string directory = Path.Combine(o.Root, "_Archive");
        OrdinaryPath(o.Root, directory);
        Directory.CreateDirectory(directory);
        string pathName = Path.Combine(directory, "ModelBundleBefore_" + DateTime.Now.ToString("yyyyMMdd_HHmmss_fff", CultureInfo.InvariantCulture) + "_" + Guid.NewGuid().ToString("N").Substring(0, 8) + ".zip");
        using (FileStream stream = new FileStream(pathName, FileMode.CreateNew))
        using (ZipArchive archive = new ZipArchive(stream, ZipArchiveMode.Create))
            foreach (KeyValuePair<string, byte[]> item in files)
                using (Stream entry = archive.CreateEntry(item.Key, CompressionLevel.Optimal).Open()) entry.Write(item.Value, 0, item.Value.Length);
        using (FileStream stream = File.OpenRead(pathName))
        using (ZipArchive archive = new ZipArchive(stream, ZipArchiveMode.Read))
        {
            if (archive.Entries.Count != files.Count) throw new IOException("Backup entry count mismatch.");
            foreach (ZipArchiveEntry entry in archive.Entries)
                using (Stream input = entry.Open())
                using (MemoryStream copy = new MemoryStream())
                {
                    input.CopyTo(copy);
                    if (!Equal(copy.ToArray(), files[entry.FullName])) throw new IOException("Backup byte verification failed.");
                }
        }
        foreach (KeyValuePair<string, byte[]> item in files)
            if (!Equal(File.ReadAllBytes(Path.Combine(o.Output, item.Key)), item.Value)) throw new IOException("Output changed during backup.");
        Say("Verified previous-release backup: " + pathName);
    }

    private static byte[] Read(string path)
    {
        path = Path.GetFullPath(path);
        byte[] bytes = File.ReadAllBytes(path);
        string hash = Sha(bytes), previous;
        if (Inputs.TryGetValue(path, out previous) && previous != hash) throw new IOException("Input changed while reading: " + path);
        Inputs[path] = hash;
        return bytes;
    }

    private static void CheckInputs()
    {
        foreach (KeyValuePair<string, string> input in Inputs)
            if (Sha(File.ReadAllBytes(input.Key)) != input.Value) throw new IOException("Input changed during build; finish saving DSMT first: " + input.Key);
    }

    private static Dictionary<string, object> ReadJson(string path) { return JsonObject(Read(path)); }
    private static Dictionary<string, object> JsonObject(byte[] bytes)
    {
        int start = bytes.Length >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf ? 3 : 0;
        return Dict(Json.DeserializeObject(Utf8.GetString(bytes, start, bytes.Length - start)));
    }
    private static string Utf16(byte[] bytes)
    {
        if (bytes.Length < 2 || !((bytes[0] == 0xff && bytes[1] == 0xfe) || (bytes[0] == 0xfe && bytes[1] == 0xff)))
            throw new InvalidDataException("Native group export must have a UTF-16 BOM.");
        return new UnicodeEncoding(bytes[0] == 0xfe, true, true).GetString(bytes, 2, bytes.Length - 2);
    }
    private static Dictionary<string, object> Dict(object value)
    {
        Dictionary<string, object> result = value as Dictionary<string, object>;
        if (result == null) throw new InvalidDataException("Expected a JSON object.");
        return result;
    }
    private static object[] ArrayValue(object value)
    {
        object[] result = value as object[];
        if (result == null) throw new InvalidDataException("Expected a JSON array.");
        return result;
    }
    private static string Text(Dictionary<string, object> data, string key)
    {
        object value;
        if (!data.TryGetValue(key, out value) || !(value is string)) throw new InvalidDataException("Missing string field: " + key);
        return (string)value;
    }
    private static int Number(object value)
    {
        if (!(value is int) && !(value is long)) throw new InvalidDataException("MapID must be an integer.");
        return Convert.ToInt32(value, CultureInfo.InvariantCulture);
    }
    private static Dictionary<string, object> D(params object[] fields)
    {
        Dictionary<string, object> result = new Dictionary<string, object>();
        for (int i = 0; i < fields.Length; i += 2) result.Add((string)fields[i], fields[i + 1]);
        return result;
    }
    private static string Key(params object[] fields) { return Json.Serialize(fields); }
    private static void Count(Dictionary<string, int> counts, string key)
    {
        int value; counts.TryGetValue(key, out value); counts[key] = value + 1;
    }
    private static bool SameCounts(Dictionary<string, int> a, Dictionary<string, int> b)
    {
        if (a.Count != b.Count) return false;
        foreach (KeyValuePair<string, int> item in a)
        {
            int value; if (!b.TryGetValue(item.Key, out value) || value != item.Value) return false;
        }
        return true;
    }
    private static bool Equal(byte[] a, byte[] b)
    {
        if (a == null || b == null) return a == b;
        if (a.Length != b.Length) return false;
        for (int i = 0; i < a.Length; ++i) if (a[i] != b[i]) return false;
        return true;
    }
    private static string Sha(byte[] bytes)
    {
        using (SHA256 algorithm = SHA256.Create()) return BitConverter.ToString(algorithm.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
    }
    private static uint Crc32(byte[] bytes)
    {
        uint crc = 0xffffffffU;
        foreach (byte value in bytes)
        {
            crc ^= value;
            for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) != 0 ? 0xedb88320U : 0U);
        }
        return crc ^ 0xffffffffU;
    }
    private static string Hex(uint value) { return "0x" + value.ToString("X8", CultureInfo.InvariantCulture); }
    private static string FullPath(string root, string path) { return Path.GetFullPath(Path.IsPathRooted(path) ? path : Path.Combine(root, path)); }
    private static void RequireInside(string root, string path)
    {
        string prefix = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        if (!Path.GetFullPath(path).StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Path must be inside " + root);
    }
    private static string Relative(string root, string path)
    {
        RequireInside(root, path);
        return Path.GetFullPath(path).Substring(Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar).Length + 1);
    }
    private static void OrdinaryPath(string root, string path)
    {
        string current = Path.GetFullPath(path);
        while (current != null)
        {
            if ((File.Exists(current) || Directory.Exists(current)) && (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidDataException("Reparse-point paths are not supported: " + current);
            if (String.Equals(current, Path.GetFullPath(root), StringComparison.OrdinalIgnoreCase)) break;
            current = Path.GetDirectoryName(current);
        }
    }
    private static void Say(string value) { Console.WriteLine(value); Log.AppendLine(value); }
    private static string Pretty(string source)
    {
        StringBuilder result = new StringBuilder();
        bool quoted = false, escaped = false;
        int depth = 0;
        foreach (char c in source)
        {
            if (quoted)
            {
                result.Append(c);
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') quoted = false;
                continue;
            }
            if (c == '"') { quoted = true; result.Append(c); }
            else if (c == '{' || c == '[') { result.Append(c); result.Append('\n'); result.Append(' ', ++depth * 2); }
            else if (c == '}' || c == ']') { result.Append('\n'); result.Append(' ', --depth * 2); result.Append(c); }
            else if (c == ',') { result.Append(c); result.Append('\n'); result.Append(' ', depth * 2); }
            else if (c == ':') result.Append(": ");
            else result.Append(c);
        }
        return result.ToString();
    }
}
