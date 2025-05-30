import React, {useMemo} from "react";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "./gl/test.vert";
import Frag from "./gl/test.frag";
import FlatVert from "./gl/flat.vert";
import FlatFrag from "./gl/flat.frag";
import GLProgramDrawMode from "../../hooks/webgl/program/GLProgramDrawMode.ts";
import useAttribute, {vecArray} from "../../hooks/webgl/program/attribute/useAttribute.ts";
import useUniform from "../../hooks/webgl/program/uniform/useUniform.ts";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import {max} from "lodash";
import GLState from "../../hooks/webgl/gl/GLState.ts";

export interface UVVisSpecGLGraphProps {
  // The data points to plot
  luminance: number[],
  wavelengthLineCount: number,
  percentageLineCount: number,

  startWavelength: number,
  endWavelength: number,

  onMouseMove: (event: React.MouseEvent<HTMLCanvasElement, MouseEvent>) => void,
  gl: GLState,
}

const UVVisSpecGLGraph: React.FC<UVVisSpecGLGraphProps> = (props) => {
  const gl = props.gl;

  const luminance = props.luminance;
  const maxLuminance = useMemo(() => Math.max(max(luminance) ?? 441.67295593, 10), [luminance]);

  // Placeholder data
  const data = useMemo(() => luminance.map((value, index) => [index, value / maxLuminance]), [luminance, maxLuminance])

  const points = useMemo(() => (
    luminance.map((v, i) => {
      const x = 2 * i / (luminance.length - 1) - 1;
      const y = 2 * v / maxLuminance - 1

      return [x, y]
    })
  ), [luminance, maxLuminance])

  const bgHorizontalCount = props.percentageLineCount - 1;
  const bgVerticalCount = props.wavelengthLineCount -2;

  // Draw a filled in polygon below data line
  const fillProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.TRIANGLE_STRIP,
    vertexCount: data.length * 2,
  });
  useAttribute(fillProgram, "aPosition", () => (
    points.flatMap(([x, y]) => [[x, y], [x, -1]])
  ), [points]);
  useUniform(fillProgram, "uColor", () => [39/255, 39/255, 42/255, 1], [])
  useUniform(fillProgram, "uRainbowConfig", [0.125, 0.8]);
  useUniform(fillProgram, "uWavelengthLimits", [props.startWavelength, props.endWavelength]);

  // Background horizontal lines
  const bgVerticalProgram = useProgram(gl, FlatVert, FlatFrag, {
    drawMode: GLProgramDrawMode.LINES,
    vertexCount: bgVerticalCount * 2,
  });
  useAttribute(bgVerticalProgram, "aPosition", () => (
    Array.from({ length: bgVerticalCount }, (_, i) => 2 * (i+1) / (bgVerticalCount+1) - 1)
      .flatMap((x) => [[x, -1], [x, 1]])
  ), [bgVerticalCount]);
  useUniform(bgVerticalProgram, "uColor", () => [0.247, 0.247, 0.274, 1], []);
  // useUniform(bgVerticalProgram, "uRainbowConfig", [0.1, 0.8]);
  // useUniform(bgVerticalProgram, "uWavelengthLimits", [props.startWavelength, props.endWavelength]);

  // Background horizontal lines
  const bgHorizontalProgram = useProgram(gl, FlatVert, FlatFrag, {
    drawMode: GLProgramDrawMode.LINES,
    vertexCount: bgHorizontalCount * 2,
  });
  useAttribute(bgHorizontalProgram, "aPosition", () => (
    Array.from({ length: bgHorizontalCount }, (_, i) => 2 * (i+1) / (bgHorizontalCount+1) - 1)
      .flatMap((y) => [[-1, y], [1, y]])
  ), [bgHorizontalCount]);
  useUniform(bgHorizontalProgram, "uColor", () => [0.247, 0.247, 0.274, 1], []);
  // useUniform(bgHorizontalProgram, "uRainbowConfig", [0.1, 0.8]);
  // useUniform(bgHorizontalProgram, "uWavelengthLimits", [props.startWavelength, props.endWavelength]);

  // Draw a line along the data
  const lineProgram = useProgram(gl, FlatVert, FlatFrag, {
    drawMode: GLProgramDrawMode.LINE_STRIP,
    vertexCount: data.length,
  });
  useAttribute(lineProgram, "aPosition", () => points as vecArray, [points]);
  const baseBrightness = 1.0
  useUniform(lineProgram, "uColor", () => [baseBrightness, baseBrightness, baseBrightness, baseBrightness], []);
  // useUniform(lineProgram, "uRainbowConfig", [0.1, 5.0]);
  // useUniform(lineProgram, "uWavelengthLimits", [props.startWavelength, props.endWavelength]);

  // Put those together into a chart
  return (
    <AutosizedGLCanvas gl={props.gl} className="rounded border-content3 border-1 cursor-cell" onMouseMove={props.onMouseMove}>
    </AutosizedGLCanvas>
  );
}

export default UVVisSpecGLGraph;