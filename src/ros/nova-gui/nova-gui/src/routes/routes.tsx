import { RouteObject } from "react-router-dom";
import ARCExcavationConstructionView from "../views/arc/ARCEscavationConstructionView";
import ARCMappingAutonomousView from "../views/arc/ARCMappingAutonomous";
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
  cameraSetup,
} from "../views/shared/CamerasPage/CameraPageConstants.ts";
import TestWebGLView from "../views/test/TestWebGLView.tsx";

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
    path: "/arc/excavation-construction",
    element: <ARCExcavationConstructionView />,
  },
  {
    path: "/arc/mapping-autonomous",
    element: <ARCMappingAutonomousView />,
  },
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
];

export const generalRoutes: RouteObject[] = [
  {
    path: "/general/cameras",
    element: <CameraPage views={cameraSetup[ARCCompModes.POST_LANDING]} />,
  },
];

export const testRoutes: RouteObject[] = [
  {
    path: "/test/webgl",
    element: <TestWebGLView/>,
  },
];

const cameraRoutes: RouteObject[] = [
  {
    path: "/cameras",
    element: <CameraPage views={cameraSetup[ARCCompModes.POST_LANDING]} />,
  },
  ...Object.values(ARCCompModes).map<RouteObject>((comp) => ({
    path: `/cameras/${comp}`,
    element: <CameraPage views={cameraSetup[comp]} />,
  })),
  { path: "/cameras/:serial", element: <SingleCameraPage /> },
];

export const routes: RouteObject[] = [
  {
    path: "/",
    element: <Root />,
    children: [
      {
        path: "/arc",
        element: <ARCBaseView />,
        children: arcRoutes,
      },
      {
        path: "/urc",
        element: <URCBaseView />,
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
