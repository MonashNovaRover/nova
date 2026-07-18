enum GLProgramDrawMode {
  // Draws a single dot.
  POINTS = 0,
  // Draws a straight line to the next vertex.
  LINE_STRIP = 1,
  // Draws a straight line to the next vertex, and connects the last vertex back to the first.
  LINE_LOOP = 2,
  // Draws a line between a pair of vertices.
  LINES = 3,
  TRIANGLE_STRIP = 4,
  TRIANGLE_FAN = 5,
  // Draws a triangle for a group of three vertices.
  TRIANGLES = 6
}

export default GLProgramDrawMode;

export function mapDrawMode(context: WebGL2RenderingContext, mode: GLProgramDrawMode): GLint {
  switch(mode) {
    case GLProgramDrawMode.POINTS:
      return context.POINTS;
    case GLProgramDrawMode.LINES:
      return context.LINES;
    case GLProgramDrawMode.LINE_STRIP:
      return context.LINE_STRIP;
    case GLProgramDrawMode.LINE_LOOP:
      return context.LINE_LOOP;
    case GLProgramDrawMode.TRIANGLE_STRIP:
      return context.TRIANGLE_STRIP;
    case GLProgramDrawMode.TRIANGLE_FAN:
      return context.TRIANGLE_FAN;
    case GLProgramDrawMode.TRIANGLES:
      return context.TRIANGLES;
  }
}