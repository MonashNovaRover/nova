import { RouteObject } from "react-router-dom";
import ARCExcavationConstructionView from "../views/arc/ARCEscavationConstructionView";
import ARCAutonomousView from "../views/arc/ARCMappingAutonomous";
import ARCPostLandingView from "../views/arc/ARCPostLandingView";
import ARCSpaceResourcesView from "../views/arc/ARCSpaceResourcesView";
import URCBaseView from "../views/urc/URCBaseView";
import URCAutonomousNavigationView from "../views/urc/URCAutonomousNavigationView";
import {URCDeliveryView} from "../views/urc/URCDeliveryView.tsx";
import URCEquipmentServicingView from "../views/urc/URCEquipmentServicingView";
import URCScienceView from "../views/urc/URCScienceView";
import { Root } from "../root";
import { CameraView } from "../views/shared/CamerasPage/CamerasView.tsx";
import { SingleCameraPage } from "../views/shared/SingleCameraPage/SingleCameraPage.tsx";
import {
  ARCCompModes,
  URCCompModes,
  arcCameraSetup,
  urcCameraSetup,
} from "../views/shared/CamerasPage/CameraViewConstants.tsx";
import GeneralBaseView from "../views/general/GeneralBaseView.tsx";
import { ARCNIRProbeView } from "../views/arc/ARCNIRProbeView.tsx";
import TestWebGLView from "../views/test/TestWebGLView/TestWebGLView.tsx";
import URCUVVisSpecView from "../views/urc/URCUVVisSpecView.tsx";
import URC360CamView from "../views/urc/URC360CamView.tsx";
import URCRamanView from "../views/urc/URCRamanView.tsx";
import URCAutoTypingView from "../views/urc/URCAutoTypingView.tsx";
import { URCCartographerView } from "../views/urc/URCCartographerView.tsx";
import { URCDeliveryCartographerView } from "../views/urc/URCDeliveryCartographerView.tsx";
import TestStateView from "../views/test/TestStateView/TestStateView.tsx";
import TestOverlayView from "../views/test/TestOverlayView/TestOverlayView.tsx";
import URCGazeboView from "../views/urc/URCGazebo.tsx";
import HomePageView from "../views/shared/HomePageView.tsx";
import { arcNavigationData, compNavigationData, generalNavigationData, testNavigationData, urcNavigationData } from "../utils/NavigationRoutes.tsx";
import PageNotFoundView from "../views/shared/PageNotFound.tsx";
import {YoloProvider} from "../components/auto/ObjectDetection/YoloProvider.tsx";
import TestJoyView from "../views/test/TestJoyView/TestJoyView.tsx";
import TestUrdfView from "../views/test/TestUrdfView/TestUrdfView.tsx";
import URCSciencePresentationView from "../views/urc/URCSciencePresentationView.tsx";

export const arcRoutes: RouteObject[] = [
  {
    path: "/arc",
    element: <HomePageView navigationData={arcNavigationData} />,
  },
  {
    path: "/arc/post-landing",
    element: <ARCPostLandingView />,
  },
  {
    path: "/arc/space-resources",
    element: <ARCSpaceResourcesView />,
  },
  {
    path: "/arc/space-resources/nir-spectroscopy",
    element: <ARCNIRProbeView />,
  },
  {
    path: "/arc/excavation-construction",
    element: <ARCExcavationConstructionView />,
  },
  {
    path: "/arc/autonomous",
    element: <ARCAutonomousView />,
  },
  {
    path: "/arc/cameras",
    element: <CameraView views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]} />,
  },
  ...Object.values(ARCCompModes).map<RouteObject>((comp) => ({
    path: `/arc/cameras/${comp}`,
    element: <CameraView views={arcCameraSetup[comp]} defaultGridSize={comp == ARCCompModes.ARC_AUTONOMOUS ? 3 : 4}/>,
  })),
];

export const urcRoutes: RouteObject[] = [
  {
    path: "/urc",
    element: <HomePageView navigationData={urcNavigationData} />,
  },
  {
    path: "/urc/base",
    element: <URCBaseView />
  },
  {
    path: "/urc/science",
    element: <URCScienceView />
  },
  {
    path: "/urc/delivery",
    element: <URCDeliveryView />,
  },
  {
    path: "/urc/delivery/cartographer",
    element: <URCDeliveryCartographerView />,
  },
  {
    path: "/urc/equipment-servicing",
    element: <URCEquipmentServicingView />,
  },
  {
    path: "/urc/auto-typing",
    element: <URCAutoTypingView />,
  },
  {
    path: "/urc/autonomous-navigation",
    element: <URCAutonomousNavigationView />,
  },
  {
    path: "/urc/cartographer",
    element: <URCCartographerView />,
  },
  {
    path: "/urc/cameras",
    element: <CameraView views={urcCameraSetup[URCCompModes.URC_EQUIPMENT_SERVICING]} />,
  },
  {
    path: "/urc/uv-vis-spec",
    element: <URCUVVisSpecView/>
  },
  {
    path: "/urc/360cam",
    element: <URC360CamView/>
  },
  {
    path: "/urc/science-presentation",
    element: <URCSciencePresentationView/>
  },
  {
    path: "/urc/raman",
    element: <URCRamanView/>
  },
  {
    path: "/urc/gazebo",
    element: <URCGazeboView/>
  },
  ...Object.values(URCCompModes).map<RouteObject>((comp) => ({
    path: `/urc/cameras/${comp}`,
    element: comp == URCCompModes.URC_AUTONOMOUS
      ? <YoloProvider><CameraView views={urcCameraSetup[comp]}/></YoloProvider>
      : <CameraView views={urcCameraSetup[comp]} />,
  })),
];

export const generalRoutes: RouteObject[] = [
  {
    path: "/general",
    element: <HomePageView navigationData={generalNavigationData} />,
  },
  {
    path: "/general/cameras",
    element: <CameraView views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]} />,
  },
  {
    path: "/general/drive",
    element: <GeneralBaseView />,
  },
];

export const testRoutes: RouteObject[] = [
  {
    path: "/test",
    element: <HomePageView navigationData={testNavigationData} />,
  },
  {
    path: "/test/webgl",
    element: <TestWebGLView/>
  },
  {
    path: "/test/state",
    element: <TestStateView/>
  },
  {
    path: "/test/nirprobe",
    element:
      <div className="p-3">
        <ARCNIRProbeView/>
      </div>,
  },
  {
    path: "/test/overlay",
    element: <TestOverlayView/>
  },
  {
    path: "/test/joy",
    element: <TestJoyView/>
  },
  {
    path: "/test/urdf",
    element: <TestUrdfView/>
  },
];

const cameraRoutes: RouteObject[] = [
  {
    path: "/cameras",
    element: <CameraView views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]} />,
  },
  { path: "/cameras/:serial", element: <SingleCameraPage /> },
];

export const routes: RouteObject[] = [
  {
    path: "/",
    element: <Root />,
    children: [
      {
        path: "/",
        element: <HomePageView navigationData={compNavigationData} />,
      },
      {
        path: "/arc",
        children: arcRoutes,
      },
      {
        path: "/urc",
        children: urcRoutes,
      },
      {
        path: "/general",
        children: generalRoutes,
      },
      {
        path: "/test",
        children: testRoutes,
      },
      {
        path: "/cameras",
        children: cameraRoutes,
      },
      {
        path: "*",
        element: <PageNotFoundView />,
      }
    ],
  },
];
