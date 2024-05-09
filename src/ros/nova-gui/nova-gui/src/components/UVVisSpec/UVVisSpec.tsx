/**
 * UV Vis Spectrometer component
 * Author: Bailey Chessum
 */
import React, {useEffect, useMemo} from "react";
import {Card, CardBody, CardHeader} from "@nextui-org/react";
// import {getDefaultPeakFinder} from "../SpectraDisplay/ChartAnalysis.ts";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import useGL from "../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "./gl/test.vert";
import Frag from "./gl/test.frag";
import GLProgramDrawMode from "../../hooks/webgl/program/GLProgramDrawMode.ts";
import useAttribute, {vecArray} from "../../hooks/webgl/program/attribute/useAttribute.ts";
import useUniform from "../../hooks/webgl/program/uniform/useUniform.ts";


const UVVisSpec: React.FC = () => {

  const bifrost = useBifrost({ topic: RosTopic.UV_VIS_SPEC });
  const luminance = useSelector((state: RootState) => state.uvVisSpecStore.luminance);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Placeholder data
  const data = luminance.map((value, index) => [index, value / 4.42])

  // Construct the data into a format to be displayed by <DataChart>
  /*const apexDataOutput = [{
    name: "Webcam Slice",
    data: data
  }];*/

  const gl = useGL();

  


  const points = useMemo(() => (
    luminance.map((v, i) => {
      const x = 2 * i / (luminance.length - 1) - 1;
      const y = 2 * v / 441.67295593 - 1

      return [x, y]
    })
  ), [luminance])

  /*
    Array.from({ length: luminance.length }, (_, index) => 2 * index / (luminance.length - 1) - 1)
      .flatMap((x) => [[x, 1], [x, -1]])
   */

  // Draw a filled in polygon below data line
  const fillProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.TRIANGLE_STRIP,
    vertexCount: data.length * 2,
  });
  useAttribute(fillProgram, "aPosition", () => (
    points.flatMap(([x, y]) => [[x, y], [x, -1]])
  ), [points]);
  useUniform(fillProgram, "uColor", () => [0.2, 0.2, 0.2, 0.1], [])

  // Draw a line along the data
  const lineProgram = useProgram(gl, Vert, Frag, {
    drawMode: GLProgramDrawMode.LINE_STRIP,
    vertexCount: data.length,
  });
  useAttribute(lineProgram, "aPosition", () => points as vecArray, [points]);
  useUniform(lineProgram, "uColor", () => [1., 1., 1., 1.], []);

  // Put those together into a chart
  const chart = (
    <AutosizedGLCanvas gl={gl} className="aspect-[4/3] rounded">
    </AutosizedGLCanvas>
  )

  return (
    <Card>
      <CardHeader>
        UV Vis Spec
      </CardHeader>
      <CardBody>
        {chart}
      </CardBody>
    </Card>
    // peaks={getDefaultPeakFinder(2, 20)(data)}>
  )

}

export default UVVisSpec



