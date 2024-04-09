import EffectQueue from "../effectQueue/EffectQueue.ts";

export default class ProgramEffectQueue extends EffectQueue<[WebGL2RenderingContext, WebGLProgram]> {

  private readonly glQueue: EffectQueue<[WebGL2RenderingContext]>;
  public _program?: WebGLProgram;

  private glQueuePushFrameID: number = -1;

  constructor(glQueue: EffectQueue<[WebGL2RenderingContext]>) {
    super();

    this.glQueue = glQueue;
  }

  public set program(value: WebGLProgram | undefined) {
    this._program = value;

    // If there have been items added to this queue before the program was initialised, we haven't pushed to queue yet.
    if (this.length > 0) {
      this.pushToGLQueue();
    }
  }

  /**
   * Pushes to the gl queue to try and initiate a re-render of the entire webgl context
   * @private
   */
  private pushToGLQueue() {
    this.glQueue.cancel(this.glQueuePushFrameID);
    this.glQueuePushFrameID = this.glQueue.push((gl) => {
      if (this._program !== undefined)
        this.clear(gl, this._program)
    });

    console.log("hello")
  }

  /**
   * Pushes an effect to the queue, and pushes this.clear to the gl queue. You can leave the effect empty if you just
   * want to push to the gl queue, as long as you know you will only push as fast as the requestAnimationFrame loop
   * @param effect
   */
  override push(effect?: (context: WebGL2RenderingContext, program: WebGLProgram) => void): number {
    const frameID = super.push(effect);

    // Try update the gl queue so a rerender is performed
    if (this.length <= 1) {
      this.pushToGLQueue();
    }

    return frameID;
  }
}