enum HTMLTextureFormat {
  ALPHA = 6406,
  RGB = 6407,
  RGBA = 6408,
  LUMINANCE = 6409,
  LUMINANCE_ALPHA = 6410
}

export default HTMLTextureFormat;

export function mapInternalFormat(context: WebGL2RenderingContext, internalFormat: HTMLTextureFormat): GLint {
  switch (internalFormat) {
    case HTMLTextureFormat.ALPHA:
      return context.ALPHA;
    case HTMLTextureFormat.RGB:
      return context.RGB;
    case HTMLTextureFormat.RGBA:
      return context.RGBA;
    case HTMLTextureFormat.LUMINANCE:
      return context.LUMINANCE;
    case HTMLTextureFormat.LUMINANCE_ALPHA:
      return context.LUMINANCE_ALPHA;
    default:
      throw new Error("Invalid HTMLTextureFormat");
  }
}