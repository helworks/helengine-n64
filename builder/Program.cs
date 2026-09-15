namespace helengine.n64.builder;

public static class Program {
    public static int Main(string[] args) {
        if (args.Length == 1 && string.Equals(args[0], "--describe", StringComparison.OrdinalIgnoreCase)) {
            Nintendo64PlatformAssetBuilder builder = new();
            Console.WriteLine(builder.Descriptor.BuilderId);
            Console.WriteLine(builder.Descriptor.TargetPlatformId);
            Console.WriteLine(builder.Definition.DisplayName);
            return 0;
        }

        Console.WriteLine("helengine.n64.builder --describe");
        return 0;
    }
}
