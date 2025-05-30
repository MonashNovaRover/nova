import {
  Button,
  ModalBody,
  ModalContent,
  ModalHeader,
  ScrollShadow,
} from "@nextui-org/react";
import { SidebarWrapper } from "./SidebarWrapper";
import { RootState } from "../../redux/RootState";
import { useSelector } from "react-redux";
import { useUIActions } from "../../redux/actions/useUIActions";
import novaLogo from "../../assets/nova-logo.png";
import {Aperture, Camera, Globe, Home, Image, Map} from "react-feather";
import { ReactNode } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import {URCCompModes} from "../../views/shared/CamerasPage/CameraPageConstants";

interface SidebarEntry {
  title: string;
  route: string;
  icon: ReactNode;
}

interface SidebarInterface {
  [task: string]: SidebarEntry[];
}

const urcSidebarData: SidebarInterface = {
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

const arcSidebarData: SidebarInterface = {
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

const generalSideBarData: SidebarInterface = {
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

export const NeoSidebar = () => {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const navigate = useNavigate();
  const location = useLocation();

  const currentPath = location.pathname;

  const sidebarData =
      location.pathname.startsWith("/arc") ? arcSidebarData :
      location.pathname.startsWith("/urc") ? urcSidebarData :
      location.pathname.startsWith("/general") ? generalSideBarData :
      {};

  return (
    <SidebarWrapper
      isOpen={uiState.sidebarIsVisible}
      onClose={() => uiActions.setSideBarVisibility(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-row justify-start pt-5">
          <img src={novaLogo} className="w-24" alt="Nova Logo" />
        </ModalHeader>
        <ModalBody>
          <ScrollShadow>
            {Object.keys(sidebarData).map((item) => {
              return (
                <div key={item} className="flex flex-col pb-5">
                  <div className="text-sm font-light">{item}</div>
                  <div className="flex flex-col gap-2 mt-2">
                    {sidebarData[item].map((mode) => {
                      const isCurrentSelected = currentPath === mode.route;
                      return (
                        <Button
                          onPress={() => navigate(mode.route)}
                          size="md"
                          variant={isCurrentSelected ? "solid" : "light"}
                          color={isCurrentSelected ? "primary" : "default"}
                          fullWidth
                          className="pl-3"
                          key={mode.route}
                        >
                          <div
                            className={`w-full flex flex-row justify-start gap-3 items-center m-0 ${
                              !isCurrentSelected && "text-gray-400"
                            }`}
                          >
                            <div>{mode.icon}</div>
                            <div>{mode.title}</div>
                          </div>
                        </Button>
                      );
                    })}
                  </div>
                </div>
              );
            })}
          </ScrollShadow>
        </ModalBody>
      </ModalContent>
    </SidebarWrapper>
  );
};
