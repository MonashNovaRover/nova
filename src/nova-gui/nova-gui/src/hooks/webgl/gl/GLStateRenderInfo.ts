/**
 * Additional information for some render using a GLState's render queue
 */
export default interface GLStateRenderInfo {
  // The current time in milliseconds
  milliseconds: DOMHighResTimeStamp,
  deltaMilliseconds: number
}

export const defaultGLStateRenderInfo = {
  milliseconds: 0,
  deltaMilliseconds: 0,
} as GLStateRenderInfo;