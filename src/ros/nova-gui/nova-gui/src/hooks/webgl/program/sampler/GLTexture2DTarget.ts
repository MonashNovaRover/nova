
enum GLTexture2DTarget {
  TEXTURE_2D = 3553,
  TEXTURE_CUBE_MAP_POSITIVE_X = 34069,
  TEXTURE_CUBE_MAP_NEGATIVE_X = 34070,
  TEXTURE_CUBE_MAP_POSITIVE_Y = 34071,
  TEXTURE_CUBE_MAP_NEGATIVE_Y = 34072,
  TEXTURE_CUBE_MAP_POSITIVE_Z = 34073,
  TEXTURE_CUBE_MAP_NEGATIVE_Z = 34074
}
export default GLTexture2DTarget;

// https://registry.khronos.org/webgl/specs/latest/2.0/#TEXTURE_TYPES_FORMATS_FROM_DOM_ELEMENTS_TABLE
export function mapTexture2DTarget(context: WebGL2RenderingContext, targetType: GLTexture2DTarget): GLint {
  switch (targetType) {
    case GLTexture2DTarget.TEXTURE_2D:
      return context.TEXTURE_2D;
    case GLTexture2DTarget.TEXTURE_CUBE_MAP_POSITIVE_X:
      return context.TEXTURE_CUBE_MAP_POSITIVE_X;
    case GLTexture2DTarget.TEXTURE_CUBE_MAP_NEGATIVE_X:
      return context.TEXTURE_CUBE_MAP_NEGATIVE_X;
    case GLTexture2DTarget.TEXTURE_CUBE_MAP_POSITIVE_Y:
      return context.TEXTURE_CUBE_MAP_POSITIVE_Y;
    case GLTexture2DTarget.TEXTURE_CUBE_MAP_NEGATIVE_Y:
      return context.TEXTURE_CUBE_MAP_NEGATIVE_Y;
    case GLTexture2DTarget.TEXTURE_CUBE_MAP_POSITIVE_Z:
      return context.TEXTURE_CUBE_MAP_POSITIVE_Z;
    case GLTexture2DTarget.TEXTURE_CUBE_MAP_NEGATIVE_Z:
      return context.TEXTURE_CUBE_MAP_NEGATIVE_Z;
    default:
      throw new Error("Invalid GLTexture2DTargetType");
  }
}