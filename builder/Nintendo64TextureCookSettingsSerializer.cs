using System.Text.Json;
using helengine.editor;

namespace helengine.n64.builder;

/// <summary>
/// Serializes and parses the N64 texture cook settings payload carried by builder-owned platform cook work items.
/// </summary>
public static class Nintendo64TextureCookSettingsSerializer {
    /// <summary>
    /// Serializes one N64 texture settings payload into the stable string form stored on cook work items.
    /// </summary>
    /// <param name="settings">Settings payload to serialize.</param>
    /// <returns>Serialized N64 texture settings string.</returns>
    public static string Serialize(TextureAssetProcessorSettings settings) {
        Validate(settings);
        Dictionary<string, object> properties = new() {
            ["maxResolution"] = settings.MaxResolution,
            ["colorFormat"] = settings.ColorFormat.ToString(),
            ["alphaPrecision"] = settings.AlphaPrecision.ToString()
        };
        if (!string.IsNullOrEmpty(settings.IndexingMethodId)) {
            properties.Add("indexingMethod", settings.IndexingMethodId);
        }
        return JsonSerializer.Serialize(properties);
    }

    /// <summary>
    /// Parses one N64 texture settings payload from the string form stored on cook work items. The
    /// <c>maxResolution</c>, <c>colorFormat</c>, and <c>alphaPrecision</c> properties are all required; a payload
    /// missing any of them is a malformed N64 texture cook settings payload and is rejected rather than
    /// silently defaulted.
    /// </summary>
    /// <param name="serializedSettings">Serialized settings string.</param>
    /// <returns>Parsed N64 texture settings payload.</returns>
    public static TextureAssetProcessorSettings Deserialize(string serializedSettings) {
        if (string.IsNullOrWhiteSpace(serializedSettings)) {
            throw new ArgumentException("Serialized N64 texture settings are required.", nameof(serializedSettings));
        }

        using JsonDocument document = JsonDocument.Parse(serializedSettings);
        JsonElement root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object) {
            throw new InvalidOperationException("N64 texture settings must be a JSON object.");
        }
        HashSet<string> propertyNames = new(StringComparer.Ordinal);
        foreach (JsonProperty property in root.EnumerateObject()) {
            if (!propertyNames.Add(property.Name)) {
                throw new InvalidOperationException($"Duplicate N64 texture setting '{property.Name}'.");
            }
        }
        if (!root.TryGetProperty("maxResolution", out JsonElement maxResolutionElement)) {
            throw new InvalidOperationException("Malformed N64 texture cook settings payload: 'maxResolution' is required.");
        }
        if (!root.TryGetProperty("colorFormat", out JsonElement colorFormatElement)) {
            throw new InvalidOperationException("Malformed N64 texture cook settings payload: 'colorFormat' is required.");
        }
        if (!root.TryGetProperty("alphaPrecision", out JsonElement alphaPrecisionElement)) {
            throw new InvalidOperationException("Malformed N64 texture cook settings payload: 'alphaPrecision' is required.");
        }

        int maxResolution = maxResolutionElement.GetInt32();
        string colorFormatName = colorFormatElement.GetString();
        string alphaPrecisionName = alphaPrecisionElement.GetString();

        if (!Enum.TryParse(colorFormatName, true, out TextureAssetColorFormat colorFormat)
            || !string.Equals(colorFormatName, colorFormat.ToString(), StringComparison.OrdinalIgnoreCase)) {
            throw new InvalidOperationException($"Unsupported N64 texture color format '{colorFormatName}'.");
        }
        if (!Enum.TryParse(alphaPrecisionName, true, out TextureAssetAlphaPrecision alphaPrecision)
            || !string.Equals(alphaPrecisionName, alphaPrecision.ToString(), StringComparison.OrdinalIgnoreCase)) {
            throw new InvalidOperationException($"Unsupported N64 texture alpha precision '{alphaPrecisionName}'.");
        }

        TextureAssetProcessorSettings settings = new() {
            MaxResolution = maxResolution,
            ColorFormat = colorFormat,
            AlphaPrecision = alphaPrecision,
            IndexingMethodId = root.TryGetProperty("indexingMethod", out JsonElement indexingMethodElement)
                ? indexingMethodElement.GetString()
                : string.Empty
        };
        Validate(settings);
        return settings;
    }

    /// <summary>
    /// Rejects unsupported formats, alpha policies and indexing methods before any source or output is mutated.
    /// Empty indexed methods retain the shared processor's QuantizedIndexed compatibility default.
    /// </summary>
    /// <param name="settings">Authored settings whose complete combination must be supported.</param>
    public static void Validate(TextureAssetProcessorSettings settings) {
        if (settings == null) {
            throw new ArgumentNullException(nameof(settings));
        }
        if (settings.MaxResolution <= 0) {
            throw new InvalidOperationException("N64 textures require a positive maximum resolution.");
        }
        if (!TextureAssetProcessorSettings.TryResolveGenericColorFormat(settings.ColorFormatId, out TextureAssetColorFormat format)
            || (format != TextureAssetColorFormat.Rgba32 && format != TextureAssetColorFormat.Indexed4 && format != TextureAssetColorFormat.Indexed8)) {
            throw new InvalidOperationException($"Unsupported N64 texture color format '{settings.ColorFormatId}'.");
        }
        TextureAssetAlphaPrecision alpha = settings.AlphaPrecision;
        if (alpha != TextureAssetAlphaPrecision.Opaque && alpha != TextureAssetAlphaPrecision.A8
            && (format == TextureAssetColorFormat.Rgba32 || (alpha != TextureAssetAlphaPrecision.Binary && alpha != TextureAssetAlphaPrecision.A4))) {
            throw new InvalidOperationException($"Unsupported N64 texture alpha precision '{alpha}' for '{format}'.");
        }
        if (format == TextureAssetColorFormat.Rgba32) {
            if (!string.IsNullOrEmpty(settings.IndexingMethodId)) {
                throw new InvalidOperationException("An indexing method requires an indexed N64 texture format.");
            }
        } else {
            settings.ResolveIndexingMethod();
        }
    }
}
