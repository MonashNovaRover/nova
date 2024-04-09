import {RenderQueueItem} from "../gl/RenderQueue.ts";
import EffectQueue from "../effectQueue/EffectQueue.ts";
import initShaderProgram from "../../webgl-utils/initShaderProgram.ts";
import {CanvasWithGL} from "../gl/useGL.tsx";

export default class GLProgramState implements RenderQueueItem {
  private program?: WebGLProgram;
  public readonly queue: EffectQueue<[WebGL2RenderingContext, WebGLProgram]>;

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

    this.queue = new EffectQueue<[WebGL2RenderingContext, WebGLProgram]>();

    this.renderQueueID = gl.renderQueue.push(this);
  }

  public setShaders(gl: CanvasWithGL, vert: string, frag: string): void {
    // This condition is to prevent the program from being immediately recompiled by the use effect in useProgram.
    if (this.vert === vert && this.frag === frag)
      return;

    this.vert = vert;
    this.frag = frag;

    gl.renderQueue.update(this.renderQueueID);
  }

  public render(context: WebGL2RenderingContext): void {
    if (this.program === undefined)
      throw Error("RenderQueue tried to render GLProgramState without a program. Did you call RenderQueueItem.setup?");

    this.queue.clear(context, this.program);

    context.useProgram(this.program);
    context.drawArrays(context.TRIANGLE_STRIP, 0, this.numberOfVertices);
  }

  public setup(context: WebGL2RenderingContext): void {
    this.program = initShaderProgram(context, this.vert, this.frag);
  }

}