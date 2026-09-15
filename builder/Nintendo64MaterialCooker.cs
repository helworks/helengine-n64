using System.Globalization;
using helengine.baseplatform.Requests;
using helengine.baseplatform.Results;

namespace helengine.n64.builder;

/// <summary>Cooks the single opaque, unlit N64 material schema.</summary>
public sealed class Nintendo64MaterialCooker {
    public PlatformMaterialCookResult Cook(PlatformMaterialCookRequest request) {
        if (request == null) throw new ArgumentNullException(nameof(request));
        if (!string.Equals(request.TargetPlatformId, "n64", StringComparison.OrdinalIgnoreCase)) {
            throw new InvalidOperationException($"N64 cannot cook materials for target platform '{request.TargetPlatformId}'.");
        }
        if (!string.Equals(request.SchemaId, Nintendo64MaterialSchemaIds.UnlitSchemaId, StringComparison.OrdinalIgnoreCase)) {
            throw new InvalidOperationException($"N64 does not support material schema '{request.SchemaId}'.");
        }

        string texture = Resolve(request.FieldValues, Nintendo64MaterialSchemaIds.TextureRelativePathFieldId);
        if (!string.IsNullOrWhiteSpace(texture)) throw new InvalidOperationException("N64 n64-unlit-v1 does not support texture bindings.");
        string lighting = Resolve(request.FieldValues, Nintendo64MaterialSchemaIds.LightingModeFieldId, "unlit");
        if (!string.Equals(lighting, "unlit", StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("N64 n64-unlit-v1 requires unlit lighting.");
        string transparency = Resolve(request.FieldValues, Nintendo64MaterialSchemaIds.TransparencyFieldId, "opaque");
        if (!string.Equals(transparency, "opaque", StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("N64 n64-unlit-v1 requires opaque materials.");

        ParseColor(Resolve(request.FieldValues, Nintendo64MaterialSchemaIds.BaseColorFieldId, "#FFFFFFFF"), out byte r, out byte g, out byte b, out byte a);
        if (a != 255) throw new InvalidOperationException("N64 n64-unlit-v1 requires an opaque base color (alpha FF).");

        helengine.PlatformMaterialAsset cooked = new() {
            Id = request.MaterialAssetId,
            RendererFamilyId = string.IsNullOrWhiteSpace(request.SelectedGraphicsProfileId) ? "n64-unlit" : request.SelectedGraphicsProfileId,
            TextureRelativePath = string.Empty,
            DoubleSided = false,
            UseVertexColor = false,
            Lit = false,
            BaseColorR = r,
            BaseColorG = g,
            BaseColorB = b,
            BaseColorA = a
        };
        return new PlatformMaterialCookResult(helengine.files.AssetSerializer.SerializeToBytes(cooked), []);
    }

    static string Resolve(IReadOnlyDictionary<string, string> values, string key, string fallback = "") {
        if (values == null) throw new ArgumentNullException(nameof(values));
        return values.TryGetValue(key, out string value) && !string.IsNullOrWhiteSpace(value) ? value.Trim() : fallback;
    }

    static void ParseColor(string value, out byte r, out byte g, out byte b, out byte a) {
        if (string.IsNullOrWhiteSpace(value)) throw new InvalidOperationException("N64 base color is required.");
        string normalized = value.Trim();
        if (normalized.StartsWith('#')) normalized = normalized[1..];
        if (normalized.Length == 6) normalized += "FF";
        if (normalized.Length != 8) throw new InvalidOperationException($"N64 color '{value}' must use #RRGGBB or #RRGGBBAA format.");
        try {
            r = byte.Parse(normalized.AsSpan(0, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            g = byte.Parse(normalized.AsSpan(2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            b = byte.Parse(normalized.AsSpan(4, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            a = byte.Parse(normalized.AsSpan(6, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        } catch (FormatException exception) {
            throw new InvalidOperationException($"N64 color '{value}' contains invalid hexadecimal channels.", exception);
        }
    }
}
