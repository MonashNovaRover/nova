enum GLWrapMode {
  REPEAT = 10497,
  CLAMP_TO_EDGE = 33071,
  MIRRORED_REPEAT = 33648,
}

export default GLWrapMode;

export function mapWrapMode(context: WebGL2RenderingContext, wrapMode: GLWrapMode): GLint {
  switch (wrapMode) {
    case GLWrapMode.REPEAT:
      return context.REPEAT;
    case GLWrapMode.CLAMP_TO_EDGE:
      return context.CLAMP_TO_EDGE;
    case GLWrapMode.MIRRORED_REPEAT:
      return context.MIRRORED_REPEAT;
    default:
      throw new Error("Invalid GLTextureWrapMode");
  }
}
