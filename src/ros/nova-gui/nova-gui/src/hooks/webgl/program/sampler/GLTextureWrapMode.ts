enum GLTextureWrapMode {
  REPEAT = 10497,
  CLAMP_TO_EDGE = 33071,
  MIRRORED_REPEAT = 33648,
}

export default GLTextureWrapMode;

export function mapTextureWrapMode(context: WebGL2RenderingContext, wrapMode: GLTextureWrapMode): GLint {
  switch (wrapMode) {
    case GLTextureWrapMode.REPEAT:
      return context.REPEAT;
    case GLTextureWrapMode.CLAMP_TO_EDGE:
      return context.CLAMP_TO_EDGE;
    case GLTextureWrapMode.MIRRORED_REPEAT:
      return context.MIRRORED_REPEAT;
    default:
      throw new Error("Invalid GLTextureWrapMode");
  }
}
