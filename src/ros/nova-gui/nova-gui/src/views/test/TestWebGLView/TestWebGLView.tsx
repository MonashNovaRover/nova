import {useCallback, useEffect, useMemo, useState} from "react";
import {Button} from "@nextui-org/react";
import useGL from "../../../components/WebGLCanvas/hooks/useGL.tsx";
import useCanvasSize from "../../../components/WebGLCanvas/hooks/useCanvasSize.tsx";
import {useProgram} from "../../../components/WebGLCanvas/hooks/program/useProgram.tsx";
import useAttributes from "../../../components/WebGLCanvas/hooks/useAttributes.tsx";
import Vert from "./test.vert";
import Frag from "./test.frag";
import useImageTexture from "../../../components/WebGLCanvas/hooks/useImageTexture.ts";
import ImageSRC from "../../../assets/nova-logo.png";
import useSampler from "../../../components/WebGLCanvas/hooks/useSampler.ts";
import useEffectQueue from "../../../components/WebGLCanvas/hooks/useEffectQueue.ts";
import useEffectQueueEffect from "../../../components/WebGLCanvas/hooks/useEffectQueueEffect.ts";
import {vec2} from "../../../components/WebGLCanvas/hooks/useUniforms.tsx";
import useUniform from "../../../components/WebGLCanvas/hooks/program/useUniform.ts";

const attributes = {
  aPosition: {
    numComponents: 2,
    data: [1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, -1.0]
  }
}

export default function TestWebGLView() {
  const fluentHooks = {
    usePrint: (...data: unknown[]) => {
      useEffect(() => {
        console.log(...data);
      }, [data]);

      return {
        usePrint: () => {
          useEffect(() => {
            console.log(...data);
          }, [data]);
        }
      }
    },
    useCounter: () : [number, ()=>void] => {
      const [count, setCount] = useState<number>(0);

      return [count, useCallback(() => setCount(i => i + 1), [setCount])];
    }
  }

  const [count, increment] = fluentHooks.useCounter();

  // fluentHooks.usePrint("current count:", count).usePrint();

  const button = <Button onPress={increment}>Increment Count</Button>;

  const gl = useGL();
  const resolution = useCanvasSize(gl);


  const program = useProgram(gl, Vert, Frag);

  useAttributes(program, attributes);
  const image = useImageTexture(ImageSRC);
  const frameID = useSampler(program, image, 0, "image");

  useEffect(() => {
    if (!gl.gl || !program)
      return;

    // Redraweeeeee
    gl.gl.clear(gl.gl.COLOR_BUFFER_BIT);
    gl.gl.useProgram(program.program);
    gl.gl.drawArrays(gl.gl.TRIANGLE_STRIP, 0, 4);
  }, [gl, program, frameID]);

  const queue = useEffectQueue();

  /*useEffectQueueEffect(() => {
    console.log("Effect queue count: ", count);
  }, [count], queue.push)
  */

  /*const [thing, setThing] = useState<number>(0);

  useEffect(() => {
    setThing(i => i + 1);
  }, [count]);

  useEffect(() => {
    setThing(i => i + 1);
  }, [count]);

  useEffect(() => {
    console.log(thing);
  }, [thing]);*/

  useEffectQueueEffect(() => {
    console.log(count);
  }, [count], queue.push);



  useEffect(() => {
    let frameID = 0;

    const callback = () => {

      if (!queue.clear() && program !== undefined) {
        // Redraweeeeee
        program.gl.clear(program.gl.COLOR_BUFFER_BIT);
        program.gl.useProgram(program.program);
        program.gl.drawArrays(program.gl.TRIANGLE_STRIP, 0, 4);
        console.log("yeah")
      }
      else {
        console.log("nah")
      }

      frameID = requestAnimationFrame(callback);
    }
    callback();

    return () => {
      cancelAnimationFrame(frameID)
    }
  }, [program, queue]);

  // useUniform(program, "offset", useMemo(() => [count / 10, 0], [count]));

  useEffectQueueEffect(() => {
    if (!program)
      return;

    const location = program.gl.getUniformLocation(program.program, "offset");

    if (location === null)
      return;

    const uniform: vec2 = [count / 10, 0];
    program.gl.uniform2f(location, ...uniform);
    console.log(uniform);

  }, [count], queue.push);

  /*useEffect(() => {
    if (queue.clear())
      return;

    console.log("queue cleared!!!");
  }, [queue])*/

  return (
    <>
      <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-6 overflow-clip max-h-screen">
        <p>{count}</p>
        <div className="relative">
          <div className="absolute top-0 left-0 right-0 bottom-0">
            <canvas width={resolution[0]} height={resolution[1]} ref={gl.canvasRef}></canvas>
          </div>
        </div>
        {button}

      </div>
    </>
  )
}
