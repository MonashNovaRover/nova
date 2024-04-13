import RenderQueue, {RenderQueueItem} from "../../render-queue/RenderQueue.ts";
import GLProgramState from "../GLProgramState.ts";
import GLTexture2DTargetType, {mapTexture2DTargetType} from "./GLTexture2DTargetType.ts";
import HTMLTextureFormat, {mapInternalFormat} from "./HTMLTextureFormat.ts";
import GLTextureWrapMode, {mapTextureWrapMode} from "./GLTextureWrapMode.ts";
import MappedGLint from "../MappedGLint.ts";
import ProgramEffectQueue from "../ProgramEffectQueue.ts";
import "rvfc-polyfill";

export interface GLSamplerStateOptions {
  target: GLTexture2DTargetType;
  format: HTMLTextureFormat;
  wrapT: GLTextureWrapMode;
  wrapS: GLTextureWrapMode;
}

export type HTMLSamplerSource = HTMLVideoElement | HTMLImageElement | null;

export default class GLSamplerState implements RenderQueueItem<[WebGL2RenderingContext, WebGLProgram]> {
  private readonly programEffectQueue: ProgramEffectQueue;
  private readonly programRenderEffectQueue: RenderQueue<[WebGL2RenderingContext, WebGLProgram]>;

  private _sampler?: HTMLImageElement | HTMLVideoElement;
  private texture?: WebGLTexture;

  public readonly target: MappedGLint<GLTexture2DTargetType>;
  public readonly format: MappedGLint<HTMLTextureFormat>;
  public readonly wrapT: MappedGLint<GLTextureWrapMode>;
  public readonly wrapS: MappedGLint<GLTextureWrapMode>;

  private readonly textureUnit: number;

  private readonly name: string;

  private readonly renderQueueID: number;

  // The result of requestVideoFrameCallback in this.startVideoFrameLoop
  private frameID?: number;

  constructor(program: GLProgramState, textureUnit: number, name: string,
              sampler: HTMLImageElement | HTMLVideoElement | null | undefined, options: GLSamplerStateOptions) {
    this.textureUnit = textureUnit;
    this.name = name;
    this._sampler = sampler ?? undefined;

    this.target = new MappedGLint(mapTexture2DTargetType, options.target);
    this.format = new MappedGLint(mapInternalFormat, options.format);
    this.wrapT = new MappedGLint(mapTextureWrapMode, options.wrapT);
    this.wrapS = new MappedGLint(mapTextureWrapMode, options.wrapS);

    this.programEffectQueue = program.queue;
    this.programRenderEffectQueue = program.renderQueue;
    this.renderQueueID = program.renderQueue.push(this);
  }

  set sampler(value: HTMLImageElement | HTMLVideoElement | null | undefined) {
    // Check if there is a video frame loop that needs to stop
    if (this.frameID !== undefined) {
      if (this._sampler instanceof HTMLVideoElement)
        this._sampler.cancelVideoFrameCallback(this.frameID);
      this.frameID = undefined;
    }

    this._sampler = value ?? undefined;
    // Request an update from the render queue, such that the new texture can be assigned for this new sampler source.
    this.programRenderEffectQueue.update(this.renderQueueID);
    this.programEffectQueue.push();
  }

  render(context: WebGL2RenderingContext): void {
    if (!this.texture)
      return;

    this.ensureValuesAreMapped(context);

    context.activeTexture(context.TEXTURE0 + this.textureUnit);
    context.bindTexture(this.target.value, this.texture);
  }

  setup(context: WebGL2RenderingContext, program: WebGLProgram): void {
    if (!this._sampler)
      return;

    // Try set up the texture if it doesnt already exist
    if (!this.texture)
      this.initTexture(context);
    if (!this.texture)
      return;

    // Set up element for respective elements
    if (this._sampler instanceof HTMLVideoElement)
      this.setupVideo();
    else if (this._sampler instanceof HTMLImageElement)
      this.setupImage();

    this.ensureValuesAreMapped(context);

    const location = context.getUniformLocation(program, this.name);

    // Bind the texture to the texture unit
    context.activeTexture(context.TEXTURE0 + this.textureUnit);
    context.bindTexture(this.target.value, this.texture);

    // Tell the shader we bound the texture to the texture unit
    context.uniform1i(location, this.textureUnit);

    // console.log(`Set up TEXTURE${this.textureUnit} ${this.name}`)
  }

  private setupVideo() {
    if (!this._sampler || !(this._sampler instanceof HTMLVideoElement))
      return;

    this.startVideoFrameLoop();
  }

  private setupImage() {
    if (!this._sampler || !(this._sampler instanceof HTMLImageElement))
      return;

    if (this._sampler.complete)
      this.programEffectQueue.push((gl, program) => this.updateTextureContents(gl, program));
    else
      this._sampler.onload = () => this.programEffectQueue.push((gl, program) => this.updateTextureContents(gl, program));
  }

  /**
   * Starts the requestVideoFrameCallback loop that renders each new frame from a video
   * @private
   */
  private startVideoFrameLoop() {
    // Cancel any existing loop
    if (this.frameID !== undefined) {
      if (this._sampler instanceof HTMLVideoElement)
        this._sampler.cancelVideoFrameCallback(this.frameID);
      this.frameID = undefined;
    }

    if (!(this._sampler instanceof HTMLVideoElement))
      return;

    const callback = () => {
      if (!(this._sampler instanceof HTMLVideoElement))
        return;

      this.programEffectQueue.push((context, program) => this.updateTextureContents(context, program));

      this.frameID = this._sampler.requestVideoFrameCallback(callback);
    }

    this.frameID = this._sampler.requestVideoFrameCallback(callback);
  }

  private updateTextureContents(gl: WebGL2RenderingContext, program: WebGLProgram) {
    if (!this.texture || !this._sampler)
      return;

    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    this.ensureValuesAreMapped(gl);

    const location = gl.getUniformLocation(program, this.name)

    gl.activeTexture(gl.TEXTURE0 + this.textureUnit);
    gl.bindTexture(this.target.value, this.texture);

    gl.uniform1i(location, this.textureUnit)

    gl.texImage2D(
      this.target.value,
      0,
      this.format.value,
      this.format.value,
      gl.UNSIGNED_BYTE,
      this._sampler,
    );

    gl.texParameteri(this.target.value, gl.TEXTURE_WRAP_S, this.wrapS.value);
    gl.texParameteri(this.target.value, gl.TEXTURE_WRAP_T, this.wrapT.value);
    gl.texParameteri(this.target.value, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(this.target.value, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  }

  /**
   * Creates an empty blue texture.
   * @param context The rendering context
   * @private
   */
  private initTexture(context: WebGL2RenderingContext): boolean {
    this.texture = context.createTexture() ?? undefined;
    if (!this.texture)
      return false;

    this.target.validate(context);

    context.texImage2D(
      this.target.value,
      0,
      context.RGBA,
      1,
      1,
      0,
      context.RGBA,
      context.UNSIGNED_BYTE,
      new Uint8Array([0,0,255,255])
    );
    return true;
  }

  private ensureValuesAreMapped(context: WebGL2RenderingContext) {
    this.target.validate(context);
    this.format.validate(context);
    this.wrapT.validate(context);
    this.wrapS.validate(context);
  }

}

