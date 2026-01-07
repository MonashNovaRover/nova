import RenderQueue, {RenderQueueItem} from "../render-queue/RenderQueue.ts";
import initShaderProgram from "../../../utils/webgl/initShaderProgram.ts";
import ProgramEffectQueue from "./ProgramEffectQueue.ts";
import GLProgramDrawMode, {mapDrawMode} from "./GLProgramDrawMode.ts";
import GLState from "../gl/GLState.ts";
import GLStateRenderInfo from "../gl/GLStateRenderInfo.ts";

export interface GLProgramStateOptions {
  // Specifies the type primitive to render.
  drawMode: GLProgramDrawMode,

  // A GLsizei specifying the number of indices to be rendered when calling drawArrays
  vertexCount: number,
  // A GLint specifying the starting index in the array of vector points.
  vertexFirst: number
}

/**
 * The class that manages state for a call of useProgram.
 */
export default class GLProgramState implements RenderQueueItem<[WebGL2RenderingContext, GLStateRenderInfo]> {
  // The actual WebGL program. This should not be exposed to the user of the hooks, so they must go through the effect
  // queues when they want to modify webgl stuff.
  private program?: WebGLProgram;

  // The effect queue allowing the user to run code that depends on the WebGLProgram in sync with the render loop
  public readonly queue: ProgramEffectQueue;

  public readonly renderQueue: RenderQueue<[WebGL2RenderingContext, WebGLProgram, GLStateRenderInfo]>;

  // The vertex shader source code
  private vert: string;
  // The fragment shader source code
  private frag: string;

  // A GLsizei specifying the number of indices to be rendered when calling drawArrays
  public vertexCount: number;
  // A GLint specifying the starting index in the array of vector points.
  public vertexFirst: number;

  // Used to determine which value to set _mappedDrawMode to.
  private _drawMode: GLProgramDrawMode;
  // The actual value used by context.drawArrays
  private _mappedDrawMode?: GLint;

  // The value from gl.renderQueue.push, allowing us to perform setup() again
  private readonly renderQueueID: number;

  constructor(gl: GLState, vert: string, frag: string, options: GLProgramStateOptions) {
    this.vert = vert;
    this.frag = frag;

    this.vertexCount = options.vertexCount;
    this.vertexFirst = options.vertexFirst;
    this._drawMode = options.drawMode;

    this.queue = new ProgramEffectQueue(gl.queue);
    this.renderQueue = new RenderQueue<[WebGL2RenderingContext, WebGLProgram, GLStateRenderInfo]>();

    this.renderQueueID = gl.renderQueue.push(this);
  }

  /**
   * Mutator for the draw mode.
   * @param value The new draw mode
   */
  public set drawMode(value: GLProgramDrawMode) {
    this._drawMode = value;
    this._mappedDrawMode = undefined;
  }

  /**
   * Supplies new shaders for the program to use, likely causing a recompilation.
   * @param gl The graphics context
   * @param vert The new vertex shader source code
   * @param frag The new fragment shader source code
   */
  public setShaders(gl: GLState, vert: string, frag: string): void {
    // This condition is to prevent the program from being immediately recompiled by the use effect in useProgram.
    if (this.vert === vert && this.frag === frag)
      return;

    this.vert = vert;
    this.frag = frag;

    gl.renderQueue.update(this.renderQueueID);
  }

  /**
   * The code ran when rendering a new frame.
   * @param context The rendering context to make calls to.
   * @param info Info about the render
   */
  public render(context: WebGL2RenderingContext, info: GLStateRenderInfo): void {
    if (this.program === undefined)
      throw Error("RenderQueue tried to render GLProgramState without a program. Did you call RenderQueueItem.setup?");

    // Sets up the draw mode if not already defined
    if (this._mappedDrawMode === undefined)
      this._mappedDrawMode = mapDrawMode(context, this._drawMode);

    context.useProgram(this.program);

    this.renderQueue.render(context, this.program, info);

    context.drawArrays(this._mappedDrawMode, this.vertexFirst, this.vertexCount);
  }

  /**
   * Initializes the program when the context becomes ready to use
   * @param context The rendering context used by the program
   * @param info Info about the render
   */
  public setup(context: WebGL2RenderingContext, info: GLStateRenderInfo): void {
    if (context === null || context === undefined) {
      console.error("WebGL context is not ready yet. Skipping setup of GLProgramState.");
      return;
    }

    this.program = initShaderProgram(context, this.vert, this.frag);
    this.queue.program = this.program;

    if (this.program) {
      context.useProgram(this.program);
      this.renderQueue.setup(context, this.program, info);
    }
  }
}