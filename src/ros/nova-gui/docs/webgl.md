# WebGL Hooks


This a suite of hooks enabling the easy use of WebGL in React.

This can be a very dense topic. Please reach out to Bailey if you need help deciphering it.

I am assuming some understanding of shader programming in writing this. If you arent familiar, I'd suggest first playing
around with something like [ShaderToy](https://www.shadertoy.com/) to get an understanding of shaders. ShaderToy 
essentially allows you to write a fragment shader. With the context of rasterization, look into how vertex and fragment 
shaders interact to form a program. Heres a short [YouTube video](https://www.youtube.com/watch?v=C1ZUeHLb0YU) that \
talks about this (ignore the part about a z-buffer). Here is 
[another excerpt of a YouTube video](https://youtu.be/5W7JLgFCkwI?si=P1ojd2L9wfrElflQ&t=345) from a larger series that 
goes into more detail.

## Basic Usage 

Let's make a simple program with a single shader program, consisting of:
- a vertex shader `example.vert`,
- a fragment shader `example.frag`

you should put your shader code into `.glsl`, `.vert`, or `.frag` files. You can get IDE plugins to make these files 
look pretty :)

`example.vert`
```glsl
#version 300 es

precision mediump float;
in vec4 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;

void main() {
    gl_Position = aPosition;
    vTexCoord = aTexCoord; // vec2(0.5) + 0.5*aPosition.xy;
}
```

`example.frag`
```glsl
#version 300 es

precision mediump float;
in vec2 vTexCoord;

uniform sampler2D image;
uniform float count;

out vec4 fragColor;

void main() {
    vec2 samplePoint = vTexCoord + vec2(count/10., 0.);
    samplePoint = vec2(mod(samplePoint.x, 1.0), mod(samplePoint.y, 1.0));

    // Sample at the projected point
    vec4 texCol = texture(image, samplePoint);
    fragColor = texCol;
}
```

### Importing source code

Once you have your files, you can import them like any other file in typescript:
```ts
import Vert from "./gl/example.vert";
import Frag from "./gl/example.frag";
```
The name of the variable doesn't matter here. I've chosen `Vert` and `Frag` arbitrarily.
These will be strings containing your shader source code, which we will pass to the `useProgram` hook, which compiles 
our shaders.

### Example Component

```tsx
function ExampleComponent() {
  // Some state that is passed into uniforms/attributes
  const [count, setCount] = useState<number>(0);
  
  // Creating the context
  const gl = useGL();
  // Creating the program from the imported shader source code
  const program = useProgram(gl, Vert, Frag);

  // Configuring the data sent to the program...
  
  useScreenQuadAttribute(program, "aPosition");

  useAttribute(program, "aTexCoord", () => [
    [1 + count / 10, 1], [count / 10, 1], [1 + count / 10, 0], [count / 10, 0]
  ], [count]);

  const image = useImageTexture(imagePath);
  useSampler(program, 0, "image", image);

  useUniform(program, "count", () => [count], [count]);
  
  // Returning the output of the component, remembering to make sure the gl.canvasRef is included somewhere
  return (
    <div>
      <AutosizedGLCanvas gl={gl} className="min-h-6 min-w-6"/>
      <Button>Increment Count</Button>
    </div>
  );
}

```

## `useGL`

This is the main hook used to setup the rendering context (`WebGL2RenderingContext`) we use to make draw calls. 

Unless you know what you're doing, you probably shouldn't mess with it directly. You can pass it to the other custom 
hooks I've documented below.

Inside your function component, this should be your first WebGL hook:
```tsx
const gl = useGL();
```
Whenever you use this hook, you ***must*** define a `<canvas/>` element for it to use to generate the rendering context, 
and display your shaders on. You need to pass it the `gl.canvasRef` given by `useGL`:

```tsx
return (
  // ...
    <canvas ref={gl.canvasRef}/>
  // ...
);
```

This can be hard to manage, especially if you just want to scale your canvas about like any other HTML element, and have
the resolution of the canvas adjust accordingly.

Instead, you can use `<AutosizedGLCanvas>`, which does this for you, and makes the canvas pixel perfect. 

```tsx
return (
  // ...
    <AutosizedGLCanvas gl={gl}/>
  // ...
);
```

This will size itself like a `<div/>` element, so it might have 0 height initially. Try give it a `className="min-h-6"` 
if it doesn't seem to appear. 

See the section on `useCanvasSize` for more info.


## `useProgram`

Now, you can use a hook to compile your shader into a program!

```tsx
const program = useProgram(gl, Vert, Frag);
```

If you have more than one `useProgram` hook, the programs will be rendered on top of each other, with the last hook 
definition being rendered last.

There is an additional suite of hooks just to set up the variables defined by your program.

By default, the program will render 4 vertices with TRIANGLE_STRIP. You can configure this by passing an additional 
options object:

```ts
const program = useProgram(gl, Vert, Frag, {
  drawMode: GLProgramDrawMode.TRIANGLES,
  vertexCount: 3
})
```

## `useAttribute`

If you recall, we had some vertex attributes in `example.vert`:
```glsl
in vec4 aPosition;
in vec2 aTexCoord;
```

You can set your vertex attribute with the `useAttribute` hook, making sure you pass: 
- the program, 
- the name of your attribute, and
- either:
  1. A constant vector array 
  2. A factory function to generate the vector array, along with a dependency array

### Constant Attributes

For attributes that never vary, you can simply pass in an array of vectors as the third argument:

```tsx
useAttribute(program, "aPosition", [
  [1, 1], [-1, 1], [1, -1], [-1, -1]
]);
```

To save prevent recreating this object every single render, you give it a factory function instead:

```tsx
useAttribute(program, "aPosition", () => [
  [1, 1], [-1, 1], [1, -1], [-1, -1]
]);
```

**Note**: this is the vertex array that you would use if you simply wanted to draw a quad that fills the `<canvas/>`. This 
is extremely useful, and I use it in basically every program. It is used so frequently, there is a shorthand hook for it, which
does the same as the above example:

```ts
useScreenQuadAttribute(program);
```

### Variable Attributes

You can also have attributes that vary with your component `props`, values from `useState`, or even values from 
[bifrost](./bifrost.md)!

In this example, a variable attribute is set, which depends on some variable `offset`:

```tsx
useAttribute(program, "aTexCoord", () => [
  [1 + count, 1], [offset, 1], [1 + offset, 0], [offset, 0]
], [offset]);
```

Whenever anything in the dependencies array changes, the program is scheduled to be re-rendered.

Note: This currently only supports `float`, `vec2`, `vec3`, and `vec4` values, with each type differentiated by the 
length of the inner arrays of the attribute value. 

#### Attributes that vary with time

There is a special case for attributes that vary with time. You can use this special hook, which accesses the time 
elapsed, only available during the render process, and creates an attribute from it. If it also varies with some other 
external variable, you can pass it into a dependencies array.

You use it in the same way as `useAttribute`, but you also get access to the arguments 
`(milliseconds: DOMHighResTimeStamp, deltaMilliseconds: number)` in your factory function definition:

```ts
useTimeAttribute(lineProgram, "aLinePosition", (milliseconds) => {
  const time = milliseconds / 1000;
  return [
    [-Math.cos(time), -Math.sin(time)], [-0.5, Math.sin(0.5 * time -3.14159/4)],
    [0.5, Math.sin(0.5 * time + 3.14159/4)], [Math.cos(1.87654321 * time), Math.sin(1.87654321 * time)],
  ];
}, []);
```

## `useUniform`

If you recall, we had a uniform vec2 in `example.frag`:
```glsl
uniform float count;
```

The `useUniform` hook has the same syntax as `useAttribute`, except only takes a single vector, rather than an array of 
vectors.

```ts
useUniform(program, "count", () => [count], [count])
```

**Note**: A vector of length 1 is a `float` in glsl

#### Uniforms that vary with time

There is a special case for uniforms that vary with time. You can use this special hook, which accesses the time
elapsed, only available during the render process, and creates uniforms from it. If it also varies with some other
external variable, you can pass it into a dependencies array.

You can call this hook in several ways.

If you have a `uniform float time` in your program, you can simply use the following to set it as the elapsed time in 
seconds:

```ts
useTimeUniform(program)
```

You can specify the name of this uniform with the optional second argument:

```ts
useTimeUniform(program, "alternativeNameForTime")
```

If you have some complex uniform that varies with time, you can also pass a factory as the optional third argument. From
here, the hook can be used in the same way as `useUniform`, except you also get access to the arguments 
`(milliseconds: DOMHighResTimeStamp, deltaMilliseconds: number)` in your factory function definition:

```ts
useTimeUniform(program, "offset", (milliseconds) => [
  Math.cos(milliseconds/2000), Math.sin(milliseconds/2000)
], [])
```

Any other external dependencies can be passed in via a dependency list as the optional fourth argument.

## `useSampler`

If you recall, we had a uniform sampler in `example.frag`:
```glsl
uniform sampler2D image;
```

We can use `<img>` and `<video>` elements as sampler values for our programs. Videos will automatically re-render the 
program whenever a new frame is played too!
 
The second argument in the hook is the texture unit (which is an `int` starting from 0). This ***must*** be unique 
within the same program. You cannot have two `useSampler` hooks that have the same texture unit.

Here's an example using an image as a sampler:
```tsx
const image = useImageTexture(imagePath);

useSampler(program, 0, "image", image);
```

Here's an example using a video as a sampler:
```tsx
const videoRef = useRef<HTMLVideoElement | null>(null);
useWebcam(videoRef);

useSampler(program, 0, "image", videoRef.current);

return (
  // ...
    <video ref={videoRef} autoPlay={true}/>
  // ...
)
```
Note: video samplers only work if they are added to the DOM.

You can also specify additional options for how to treat the sampler:

```ts
useSampler(program, 0, "image", image, {
  wrapS: GLWrapMode.MIRRORED_REPEAT,        // How should the image wrap horizontally?
  wrapT: GLWrapMode.CLAMP_TO_EDGE,          // How should the image wrap vertically?
  format: HTMLTextureFormat.LUMINANCE_ALPHA // How should colours from the image be uploaded?
});
```

## More hooks

### `useResolutionUniform`

Sets a uniform of the form:

```glsl
uniform vec2 resolution;
```
to be the width and height of the canvas render target (or a sampler source!), in pixels, and is updated whenever the 
canvas (or sampler source!) is resized.
This is useful if you need to maintain aspect ratios in your shaders. 

You can use it like this, to set the resolution of the canvas to a uniform named `"resolution"`:

```ts
useResolutionUniform(gl, program);
```

You can also specify the name of the uniform, as an optional third argument:

```ts
useResolutionUniform(gl, program, "resolution")
```

Additionally, if you want to use the resolution of some `<video>` or `<image>` (for example, those used with 
`useSampler`), you can pass it as the optional fourth argument:

```ts
useResolutionUniform(gl, program, "imageResolution", image)
```


### `useProgramEffect`

This is how most of the program hooks were defined (`useAttribute` and `useUniform` use this. `useSampler` is 
complicated, and was implemented with an OOP style). 

Whenever an item in the dependencies list changes, this will run your code synchronously before the next re-render of 
the program, and schedules the program to be re-rendered.

```ts
useProgramEffect(program, (context: WebGL2RenderingContext, program: WebGLProgram) => {
  // make webgl calls using the given context and program.
}, [/* dependencies go here */])
```

You can use this to make webgl hooks that relate to the program.

Note: The decision to use the same name for the `useProgram` return value, and the argument in the callback for 
`useCallbackEffect` is intentional. You should ***never*** interact with the `useProgram` return value directly inside 
of the `useProgramEffect` callback, as it could cause infinite loops and unexpected behaviour. By hiding the 
`useProgram` return value inside of the callback, I hope to discourage users from making this mistake.

### `useProgramRenderEffect`

This defined some callback which should be run before every render of the program, but will not invoke a re-render of 
the canvas. This was useful for memory management in WebGL. For example, this was used in `useAttribute` to ensure the 
correct attribute is bound when programs are switched.

We want to avoid using this where possible, as the code you provide will run on every single render. Consider if you 
need your code to run this often before using this hook. 

```ts
useProgramRenderEffect(program, (context, program, info) => {
  // Make some webgl calls here before rendering the program.
}, [/* dependencies to the effect go here. Changes do not trigger a re-render! */])
```

### `useCanvasSize` and `<AutosizedGLCanvas/>`

Tries to automatically size your canvas element to match the size of its parent. 

If used incorrectly, the canvas can rapidly expand to a ridiculous size, and freeze up the browser tab. Have fun!

```ts
useCanvasSize(gl);
```

If you want it to match the size of some other element, you can pass it as the second argument:

```ts
useCanvasSize(gl, videoRef.current);
```

I recommend using the `<AutosizedGLCanvas>` element instead, as it handles this hook for you.

```tsx
const gl = useGL();
// ...
return (
  // ...
    <AutosizedGLCanvas gl={gl}>
    </AutosizedGLCanvas>
  // ...
);
```

Like `useCanvasSize`, this also accepts a `sizeTarget`:

```tsx
const gl = useGL();
// ...
return (
  // ...
    <AutosizedGLCanvas gl={gl} sizeTarget={videoRef.current}>
    </AutosizedGLCanvas>
  // ...
);
```

You can also give this children to be rendered on top of the canvas:

```tsx
const gl = useGL();
// ...
return (
  // ...
    <AutosizedGLCanvas gl={gl}>
      <p>Hello! I will be rendered on top of the canvas!</p>
    </AutosizedGLCanvas>
  // ...
);
```

### `useAnimationFrame`

Takes the given callback function, and puts it into a loop formed by recursively calling 
[`requestAnimationFrame`](https://developer.mozilla.org/en-US/docs/Web/API/window/requestAnimationFrame).

This is used to form the default render loop in the `useGL()` hook. 

I recommend using the `useTimeAttribute` and `useTimeUniform` hooks instead for accessing the time from your shaders.

### `useImageTexture`

Gets an image from a path, which you can use as an input to `useSampler`.

```ts
const image = useImageTexture(imagePath);
```

### `useVideoTexture`

Same as `useVideoTexture`, but for video files instead.

```ts
const videoRef = useRef<HTMLVideoElement | null>(null);
useVideoTexture(videoRef, videoPath);
```

Note: The video ref must be used in the DOM by a `<video ref={videoRef}/>` for this to work.

### `useWebcam`

Makes the given video ref display the webcam feed

```ts
const videoRef = useRef<HTMLVideoElement | null>(null);
useWebcam(videoRef);
```

Note: The video ref must be used in the DOM by a `<video ref={videoRef}/>` for this to work.
