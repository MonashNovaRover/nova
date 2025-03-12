import {Navigate, RouteObject} from "react-router-dom";
import ARCExcavationConstructionView from "../views/arc/ARCEscavationConstructionView";
import ARCAutonomousView from "../views/arc/ARCMappingAutonomous";
import ARCPostLandingView from "../views/arc/ARCPostLandingView";
import ARCSpaceResourcesView from "../views/arc/ARCSpaceResourcesView";
import ARCBaseView from "../views/arc/ARCBaseView";
import URCBaseView from "../views/urc/URCBaseView";
import URCAutonomousNavigationView from "../views/urc/URCAutonomousNavigationView";
import URCDeliveryView from "../views/urc/URCDeliveryView";
import URCEquipmentServicingView from "../views/urc/URCEquipmentServicingView";
import URCScienceView from "../views/urc/URCScienceView";
import { Root } from "../root";
import { CameraPage } from "../views/shared/CamerasPage/CamerasPage.tsx";
import { SingleCameraPage } from "../views/shared/SingleCameraPage/SingleCameraPage.tsx";
import {
  ARCCompModes,
  URCCompModes,
  arcCameraSetup,
  urcCameraSetup,
} from "../views/shared/CamerasPage/CameraPageConstants";
import GeneralBaseView from "../views/general/GeneralBaseView.tsx";
import { ARCNIRProbeView } from "../views/arc/ARCNIRProbeView.tsx";
import { ARCMicroscopeView } from "../views/arc/ARCMicroscopeView.tsx";
import TestWebGLView from "../views/test/TestWebGLView/TestWebGLView.tsx";
import URCUVVisSpecView from "../views/urc/URCUVVisSpecView.tsx";
import URC360CamView from "../views/urc/URC360CamView.tsx";
import URCRamanView from "../views/urc/URCRamanView.tsx";
import { URCCartographerView } from "../views/urc/URCCartographerView.tsx";
import TestStateView from "../views/test/TestStateView/TestStateView.tsx";
import TestOverlayView from "../views/test/TestOverlayView/TestOverlayView.tsx";

export const arcRoutes: RouteObject[] = [
  {
    path: "/arc",
    element: <ARCBaseView />,
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
    path: "/arc/space-resources/microscope",
    element: <ARCMicroscopeView />,
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
    element: <CameraPage views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]} />,
  },
  ...Object.values(ARCCompModes).map<RouteObject>((comp) => ({
    path: `/arc/cameras/${comp}`,
    element: <CameraPage views={arcCameraSetup[comp]} />,
  })),
];

export const urcRoutes: RouteObject[] = [
  {
    path: "/urc",
    element: <URCBaseView />,
  },
  {
    path: "/urc/science",
    element: <URCScienceView />,
  },
  {
    path: "/urc/delivery",
    element: <URCDeliveryView />,
  },
  {
    path: "/urc/equipment-servicing",
    element: <URCEquipmentServicingView />,
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
    element: <CameraPage views={urcCameraSetup[URCCompModes.URC_EQUIPMENT_SERVICING]} />,
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
    path: "/urc/raman",
    element: <URCRamanView/>
  },
  ...Object.values(URCCompModes).map<RouteObject>((comp) => ({
    path: `/urc/cameras/${comp}`,
    element: <CameraPage views={urcCameraSetup[comp]} />,
  })),
];

export const generalRoutes: RouteObject[] = [
  {
    path: "/general/cameras",
    element: <CameraPage views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]} />,
  },
  {
    path: "/general/drive",
    element: <GeneralBaseView />,
  },
];

export const testRoutes: RouteObject[] = [
  {
    path: "/test/webgl",
    element: <TestWebGLView/>
  },
  {
    path: "/test/state",
    element: <TestStateView/>
  },
  {
    path: "/test/overlay",
    element: <TestOverlayView/>
  },
];

const cameraRoutes: RouteObject[] = [
  {
    path: "/cameras",
    element: <CameraPage views={arcCameraSetup[ARCCompModes.ARC_POST_LANDING]} />,
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
        element: <Navigate to="/arc" />,
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
    ],
  },
];
