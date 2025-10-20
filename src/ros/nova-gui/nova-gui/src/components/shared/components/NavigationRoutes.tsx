import { ReactNode } from "react";
import { Aperture, Camera, Globe, Home, Image, Map } from "react-feather";
import { URCCompModes } from "../../../views/shared/CamerasPage/CameraPageConstants";

interface NavigationEntry {
  title: string;
  route: string;
  icon: ReactNode;
}

interface NavigationInterface {
  [task: string]: NavigationEntry[];
}

export const urcNavigationData: NavigationInterface = {
  ["Base"]: [
    {
      title: "Dashboard",
      route: "/urc",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/urc/cameras",
      icon: <Camera />,
    },
    {
      title: "Cartographer",
      route: "/urc/cartographer",
      icon: <Map />,
    },
  ],
  ["Science"]: [
    {
      title: "Dashboard",
      route: "/urc/science",
      icon: <Home />,
    },
    {
      title: "360 Cam",
      route: "/urc/360cam",
      icon: <Globe />,
    },
    {
      title: "Cameras",
      route: `/urc/cameras/${URCCompModes.URC_SCIENCE}`,
      icon: <Camera />,
    }
  ],
  ["Delivery"]: [
    {
      title: "Dashboard",
      route: "/urc/delivery",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: `/urc/cameras/${URCCompModes.URC_DELIVERY}`,
      icon: <Camera />,
    }
  ],
  ["Equipment Servicing"]: [
    {
      title: "Dashboard",
      route: "/urc/equipment-servicing",
      icon: <Home />,
    },
    {
      title: "Auto Typing",
      route: `/urc/auto-typing`,
      icon: <Camera />,
    },
    {
      title: "Cameras",
      route: `/urc/cameras/${URCCompModes.URC_EQUIPMENT_SERVICING}`,
      icon: <Camera />,
    }
  ],
  ["Autonomous"]: [
    {
      title: "Dashboard",
      route: "/urc/autonomous-navigation",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: `/urc/cameras/${URCCompModes.URC_AUTONOMOUS}`,
      icon: <Camera />,
    }
  ],
  ["Simulation"]: [
    {
      title: "Dashboard",
      route: "/urc/gazebo",
      icon: <Home />,
    },
  ],
};

export const arcNavigationData: NavigationInterface = {
  ["Post Landing"]: [
    {
      title: "Dashboard",
      route: "/arc/post-landing",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/post-landing",
      icon: <Camera />,
    },
  ],
  ["Space Resources"]: [
    {
      title: "Dashboard",
      route: "/arc/space-resources",
      icon: <Home />,
    },
    {
      title: " NIR Spectroscopy",
      route: "/arc/space-resources/nir-spectroscopy",
      icon: <Aperture />,
    },
    {
      title: "Microscope",
      route: "/arc/space-resources/microscope",
      icon: <Image />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/space-resources",
      icon: <Camera />,
    },
  ],
  ["Excavation and Construction"]: [
    {
      title: "Dashboard",
      route: "/arc/excavation-construction",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/excavation-construction",
      icon: <Camera />,
    },
  ],
  ["Autonomous"]: [
    {
      title: "Dashboard",
      route: "/arc/autonomous",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/arc/cameras/autonomous",
      icon: <Camera />,
    },

  ],
};

export const generalNavigationData: NavigationInterface = {
  ["General"]: [
    {
      title: "Dashboard",
      route: "/general/drive",
      icon: <Home />,
    },
    {
      title: "Cameras",
      route: "/general/cameras",
      icon: <Camera/>,
    },
  ]
}