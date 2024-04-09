import {RenderQueueItem} from "../gl/RenderQueue.ts";
import initShaderProgram from "../../webgl-utils/initShaderProgram.ts";
import {CanvasWithGL} from "../gl/useGL.ts";
import ProgramEffectQueue from "./ProgramEffectQueue.ts";

export default class GLProgramState implements RenderQueueItem {
  // The actual WebGL program. This should not be exposed to the user of the hooks, so they must go through the effect
  // queues when they want to modify webgl stuff.
  private program?: WebGLProgram;

  // The effect queue allowing the user to run code that depends on the WebGLProgram in sync with the render loop
  public readonly queue: ProgramEffectQueue;

  // The vertex shader source code
  private vert: string;
  // The fragment shader source code
  private frag: string;

  public numberOfVertices: number;

  // The value from gl.renderQueue.push, allowing us to perform setup() again
  private readonly renderQueueID: number;

  constructor(gl: CanvasWithGL, vert: string, frag: string, numberOfVertices: number) {
    this.vert = vert;
    this.frag = frag;
    this.numberOfVertices = numberOfVertices;

    this.queue = new ProgramEffectQueue(gl.queue);

    this.renderQueueID = gl.renderQueue.push(this);
  }

  /**
   * Supplies new shaders for the program to use, likely causing a recompilation.
   * @param gl The graphics context
   * @param vert The new vertex shader source code
   * @param frag The new fragment shader source code
   */
  public setShaders(gl: CanvasWithGL, vert: string, frag: string): void {
    // This condition is to prevent the program from being immediately recompiled by the use effect in useProgram.
    if (this.vert === vert && this.frag === frag)
      return;

    this.vert = vert;
    this.frag = frag;

    gl.renderQueue.update(this.renderQueueID);
  }

  /**
   * The code ran when rendering a new frame.
   * @param context The rendering context to make calls to
   */
  public render(context: WebGL2RenderingContext): void {
    if (this.program === undefined)
      throw Error("RenderQueue tried to render GLProgramState without a program. Did you call RenderQueueItem.setup?");

    context.useProgram(this.program);
    context.drawArrays(context.TRIANGLE_STRIP, 0, this.numberOfVertices);
  }

  /**
   * Initializes the program when the context becomes ready to use
   * @param context The rendering context used by the program
   */
  public setup(context: WebGL2RenderingContext): void {
    this.program = initShaderProgram(context, this.vert, this.frag);
    this.queue.program = this.program;
  }
}