import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../ros/topics/rosTopic.ts";
import {RosService} from "../../ros/services/rosService.ts";
import React, {useCallback, useEffect, useState} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import {CameraComponentProps} from "./CameraComponent.tsx";
import useGL from "../../hooks/webgl/gl/useGL.ts";
import useProgram from "../../hooks/webgl/program/useProgram.ts";
import Vert from "../Theta360CamWidget/gl/perspective.vert";
import Frag from "../Theta360CamWidget/gl/perspective.frag";
import useScreenQuadAttribute from "../../hooks/webgl/program/attribute/useScreenQuadAttribute.ts";
import useResolutionUniform from "../../hooks/webgl/program/uniform/useResolutionUniform.ts";
import useSampler from "../../hooks/webgl/program/sampler/useSampler.ts";
import HTMLTextureFormat from "../../hooks/webgl/program/sampler/HTMLTextureFormat.ts";
import GLWrapMode from "../../hooks/webgl/program/sampler/GLWrapMode.ts";
import useUniform, {vec} from "../../hooks/webgl/program/uniform/useUniform.ts";
import AutosizedGLCanvas from "../AutosizedGLCanvas/AutosizedGLCanvas.tsx";
import {Tooltip} from "@nextui-org/react";
import ExtendedDownloadButton from "../shared/ExtendedDownload.tsx";
import {Image} from "react-feather";
import {WebGL360CamProps} from "../Theta360CamWidget/Perspective360CamCanvas.tsx";

const ASPECT_RATIO = 4 / 3;

export interface GimbalCameraComponentProps {
  cameraSerial: string;
  autostart?: boolean;
}

//  // FUNCTION FROM /nova-gui/nova-gui/src/components/Theta360CamWidget/Perspective360CamCanvas.tsx
// const Perspective360CamCanvas: React.FC<WebGL360CamProps> = (props) => {
//   const gl = useGL();
//   const [mousePos, setMousePos] = useState([0, 0]);
//   const [fov, setFov] = useState(90);
//
//   // Allow for panning with the mouse
//   const onMouseMove = useCallback((event: React.MouseEvent<HTMLCanvasElement>) => {
//     if (event.buttons !== 1)
//       return;
//
//     const bounds = gl.canvasRef.current?.getBoundingClientRect() ?? {width: 1, height: 1};
//     const maxResolutionComp = Math.max(bounds.width, bounds.height);
//
//     setMousePos(([x, y]) => [
//       x + fov * DEG_TO_RAD * event.movementX / maxResolutionComp,
//       y + fov * DEG_TO_RAD * event.movementY / maxResolutionComp
//     ]);
//   }, [fov, gl.canvasRef]);
//
//   // Listen to scrolling to change FOV
//   const onWheel = useCallback((e: React.WheelEvent<HTMLCanvasElement>) => {
//     setFov((fov) => Math.max(Math.min((fov + e.deltaY / 50), 179), 0.01));
//   }, []);
//
//   // Create program to project and render image
//   const program = useProgram(gl, Vert, Frag);
//   useScreenQuadAttribute(program);
//   useResolutionUniform(gl, program);
//   useSampler(program, 0, "camera", props.image, {
//     format: HTMLTextureFormat.RGB,
//     wrapS: GLWrapMode.MIRRORED_REPEAT,
//     wrapT: GLWrapMode.MIRRORED_REPEAT
//   });
//   useUniform(program, "fov", () => [fov], [fov]);
//   useUniform(program, "mousePos", () => mousePos as vec, [mousePos]);
//
//
//   // Called for the screenshot button, to fetch data to put in the file to save
//   const getCanvasScreenshot = useCallback(() => {
//     if (!gl.canvasRef.current)
//       return [];
//
//     const canvas = gl.canvasRef.current;
//     gl.render(true);
//     const dataURL = canvas.toDataURL("image/png");
//
//     return convertToBlob(dataURL);
//   }, [gl])
//
//   return (
//     <AutosizedGLCanvas
//       gl={gl}
//       className="rounded p-3 flex-grow"
//       onMouseMove={onMouseMove}
//       onWheel={onWheel}
//       onMouseEnter={disableScroll}
//       onMouseLeave={enableScroll}
//     >
//       <div className="flex flex-row gap-3">
//         {props.children}
//         <Tooltip content="Take WebGL Screenshot">
//           <ExtendedDownloadButton
//             fileContent={getCanvasScreenshot}
//             filename={`360cam-perspective.png`}
//             fileType={`image/png`}
//             isIconOnly
//           >
//             <Image></Image>
//           </ExtendedDownloadButton>
//         </Tooltip>
//       </div>
//
//     </AutosizedGLCanvas>
//   )
// }


const GimbalCameraComponent: FC = (props: GimbalCameraComponentProps) => {

  //Access response from ros2 node
  const scimbalServiceData = useSelector(
    (state: RootState) => state.scimbalCamResponse
  );

  const serviceBifrost = useBifrost({ service: RosService.SCIMBAL_COMMAND});

  useEffect(() => {
    serviceBifrost.syncWithTopic();
  }, [serviceBifrost]);

  const onPress = () => {
    //do stuff on press
  }
  return(
    <CameraComponent onPress={onPress}>

    </CameraComponent>
  )
}