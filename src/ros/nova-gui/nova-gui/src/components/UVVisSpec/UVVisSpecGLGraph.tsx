import useGL from "../../hooks/webgl/gl/useGL.ts";
import React, {useMemo} from "react";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "./gl/test.vert";
import Frag from "./gl/test.frag";
import GLProgramDrawMode from "../../hooks/webgl/program/GLProgramDrawMode.ts";
import useAttribute, {vecArray} from "../../hooks/webgl/program/attribute/useAttribute.ts";
import useUniform from "../../hooks/webgl/program/uniform/useUniform.ts";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import {max} from "lodash";

export interface UVVisSpecGLGraphProps {
  // The data points to plot
  luminance: number[],
}


const UVVisSpecGLGraph: React.FC<UVVisSpecGLGraphProps> = (props) => {
  const gl = useGL();

  const luminance = props.luminance;
  const maxLuminance = Math.max(max(luminance) ?? 441.67295593, 10);

  // Placeholder data
  const data = luminance.map((value, index) => [index, value / maxLuminance])

  const points = useMemo(() => (
    luminance.map((v, i) => {
      const x = 2 * i / (luminance.length - 1) - 1;
      const y = 2 * v / maxLuminance - 1

      return [x, y]
    })
  ), [luminance])

  const bgHorizontalCount= 4
  const bgVerticalCount = 3



  // Draw a filled in polygon below data line
  const fillProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.TRIANGLE_STRIP,
    vertexCount: data.length * 2,
  });
  useAttribute(fillProgram, "aPosition", () => (
    points.flatMap(([x, y]) => [[x, y], [x, -1]])
  ), [points]);
  useUniform(fillProgram, "uColor", () => [39/255, 39/255, 42/255, 1], [])

  // Background horizontal lines
  const bgVerticalProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.LINES,
    vertexCount: bgVerticalCount * 2,
  });
  useAttribute(bgVerticalProgram, "aPosition", () => (
    Array.from({ length: bgHorizontalCount }, (_, i) => 2 * (i+1) / (bgVerticalCount+1) - 1)
      .flatMap((x) => [[x, -1], [x, 1]])
  ), [bgHorizontalCount]);
  useUniform(bgVerticalProgram, "uColor", () => [0.247, 0.247, 0.274, 1], []);


  // Background horizontal lines
  const bgHorizontalProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.LINES,
    vertexCount: bgHorizontalCount * 2,
  });
  useAttribute(bgHorizontalProgram, "aPosition", () => (
    Array.from({ length: bgHorizontalCount }, (_, i) => 2 * (i+1) / (bgHorizontalCount+1) - 1)
      .flatMap((y) => [[-1, y], [1, y]])
  ), [bgHorizontalCount]);
  useUniform(bgHorizontalProgram, "uColor", () => [0.247, 0.247, 0.274, 1], []);

  // Draw a line along the data
  const lineProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.LINE_STRIP,
    vertexCount: data.length,
  });
  useAttribute(lineProgram, "aPosition", () => points as vecArray, [points]);
  useUniform(lineProgram, "uColor", () => [1., 1., 1., 1.], []);

  // Put those together into a chart
  const chart = (
    <AutosizedGLCanvas gl={gl} className="aspect-[4/3] rounded border-content3 border-1">
    </AutosizedGLCanvas>
  )

  return chart;
}

export default UVVisSpecGLGraph;