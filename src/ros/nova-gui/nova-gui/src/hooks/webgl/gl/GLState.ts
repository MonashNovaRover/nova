import {RefObject} from "react";
import EffectQueue from "../effect-queue/EffectQueue.ts";
import RenderQueue from "../render-queue/RenderQueue.ts";

export default class GLState {
  public readonly canvasRef: RefObject<HTMLCanvasElement>;
  public _context?: WebGL2RenderingContext;
  public readonly queue: EffectQueue<[WebGL2RenderingContext]>;
  public readonly renderQueue: RenderQueue;

  constructor(canvasRef : RefObject<HTMLCanvasElement>) {
    this.canvasRef = canvasRef;

    this.renderQueue = new RenderQueue();
    this.queue = new EffectQueue<[WebGL2RenderingContext]>();
  }

  public get context() {
    return this._context;
  }

  public set context(newContext: WebGL2RenderingContext | undefined) {
    this._context = newContext;

    if (this._context === undefined)
      return;

    this.renderQueue.setup(this._context);
  }

  /**
   * Re-renders all programs if there are any changes to inputs of the programs,
   * unless otherwise specified with force = true
   * @param force Set to true if you want to force everything to be drawn, even if there are no changes to the inputs.
   */
  render(force?: boolean): void {
    if (this._context === undefined)
      return;

    if (!this.queue.clear(this._context) || force) {
      this.renderQueue.render(this._context);
    }
  }
}