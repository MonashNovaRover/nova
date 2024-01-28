import {
  Button,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownTrigger,
  Navbar,
  NavbarBrand,
  NavbarContent,
  NavbarItem,
} from "@nextui-org/react";
import { ChevronDown, Settings, HelpCircle } from "react-feather";
import novaLogo from "../../assets/nova-logo.png";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useUIActions } from "../../redux/actions/useUIActions";
import { BifrostConnectionStatus } from "../../redux/models/BifrostTypes";

const connectionStatusColor: {
  [key: string]: "success" | "warning" | "danger";
} = {
  [BifrostConnectionStatus.CONNECTED]: "success",
  [BifrostConnectionStatus.CONNECTING]: "warning",
  [BifrostConnectionStatus.DISCONNECTED]: "danger",
};

export const NovaNavbar: React.FC = () => {
  const uiActions = useUIActions();


  const bifrostStatus = useSelector(
    (state: RootState) => state.bifrostStatus.connectionStatus
  );

  // State to control the visibility of the image modal


  return (
    <Navbar maxWidth="full" isBordered position="static">
      <NavbarContent justify="start">
        <NavbarBrand>
          <img src={novaLogo} className="w-14" alt="Nova Logo" />
        </NavbarBrand>
      </NavbarContent>
      <NavbarContent as="div" className="items-center" justify="end">
        <NavbarItem>
          <Dropdown placement="bottom-end">
            <DropdownTrigger>
              <Button
                radius="sm"
                color={connectionStatusColor[bifrostStatus]}
                size="sm"
                variant="shadow"
              >
                {bifrostStatus.toString()}
              </Button>
            </DropdownTrigger>
            <DropdownMenu aria-label="ROS Connection">
              <DropdownItem>Shut Down</DropdownItem>
              <DropdownItem>Restart</DropdownItem>
              <DropdownItem>Disconnect</DropdownItem>
            </DropdownMenu>
          </Dropdown>
        </NavbarItem>
        <NavbarItem>
          <Dropdown placement="bottom-end" backdrop="blur">
            <DropdownTrigger>
              <Button
                radius="sm"
                size="sm"
                endContent={<ChevronDown className="w-4 h-4" />}
              >
                General
              </Button>
            </DropdownTrigger>
            <DropdownMenu>
              <DropdownItem
                description="General Tab for Rover Operation"
                href="/"
              >
                General
              </DropdownItem>
              <DropdownItem
                description="Australian Rover Challenge"
                href="/arc"
              >
                ARC
              </DropdownItem>
              <DropdownItem
                description="University Rover Challenge"
                href="/urc"
              >
                URC
              </DropdownItem>
            </DropdownMenu>
          </Dropdown>
        </NavbarItem>
        <NavbarItem>
          {/*Controller Help Modal*/}
          <Button 
            isIconOnly 
            radius="sm" 
            size="sm"
            variant="shadow"
            onClick={() => uiActions.setControllerHelpModal(true)}>
            <HelpCircle className="w-4 h-4 " />
          </Button>
        </NavbarItem>
        <NavbarItem>
          <Button
            isIconOnly
            size="sm"
            variant="shadow"
            onClick={() => uiActions.setSettingsModal(true)}
          >
            <Settings className="w-4 h-4" />
          </Button>
        </NavbarItem>
      </NavbarContent>
    </Navbar>
  );
};
