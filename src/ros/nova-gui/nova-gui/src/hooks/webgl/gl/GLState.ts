import {RefObject} from "react";
import EffectQueue from "../effect-queue/EffectQueue.ts";
import RenderQueue from "../render-queue/RenderQueue.ts";
import GLStateRenderInfo, {defaultGLStateRenderInfo} from "./GLStateRenderInfo.ts";

/**
 * The class that manages state for a call of useGL.
 */
export default class GLState {
  /**
   * The canvas element that this renders to.
   */
  public readonly canvasRef: RefObject<HTMLCanvasElement>;

  /**
   * The rendering context used by the GLState. This is retrieved from the canvasRef, which is made using a useRef hook,
   * and is thus initially null on the first render. So, we cannot guarantee that the context exists.
   */
  private _context?: WebGL2RenderingContext;

  /**
   * The effect queue used to trigger re-renders
   */
  public readonly queue: EffectQueue<[WebGL2RenderingContext]>;

  /**
   * The render queue, used to run a sequence of functions (usually one for each program) that do something for each
   * render (such as rendering some program to the canvas).
   */
  public readonly renderQueue: RenderQueue<[WebGL2RenderingContext, GLStateRenderInfo]>;

  constructor(canvasRef : RefObject<HTMLCanvasElement>) {
    this.canvasRef = canvasRef;

    this.renderQueue = new RenderQueue<[WebGL2RenderingContext, GLStateRenderInfo]>();
    this.queue = new EffectQueue<[WebGL2RenderingContext]>();
  }

  /**
   * Allows the context to be retrieved while still having side effects for mutating the context.
   */
  public get context() {
    return this._context;
  }

  /**
   * Mutator for the rendering context, which has the side effect of setting up everything in the renderQueue.
   * @param newContext
   */
  public set context(newContext: WebGL2RenderingContext | undefined) {
    this._context = newContext;

    if (this._context === undefined)
      return;

    this.renderQueue.setup(this._context, defaultGLStateRenderInfo);

    this.canvasRef.current!.addEventListener('webglcontextlost', (e) => {
      e.preventDefault();
    })

    this.canvasRef.current!.addEventListener('webglcontextrestored', (e) => {
      console.info('WebGL context restored, we need to recreate textures and buffers.', e);
      this.renderQueue.setup(newContext!, defaultGLStateRenderInfo);
    });
  }

  /**
   * Re-renders all programs if there are any changes to inputs of the programs,
   * unless otherwise specified with force = true
   * @param force Set to true if you want to force everything to be drawn, even if there are no changes to the inputs.
   * @param renderInfo Info about the render, including time information
   */
  render(force?: boolean, renderInfo?: GLStateRenderInfo): void {
    if (this._context === undefined)
      return;

    const filledRenderInfo = renderInfo ?? defaultGLStateRenderInfo;

    if (this.queue.clear(this._context) && !force)
      return;

    this.renderQueue.render(this._context, filledRenderInfo);
  }
}