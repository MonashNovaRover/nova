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
import { ChevronDown, Settings } from "react-feather";
import novaLogo from "../../assets/nova-logo.png";
import React, { useState } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useUIActions } from "../../redux/actions/useUIActions";
import { BifrostConnectionStatus } from "../../redux/models/BifrostTypes";
import ControllerHelpModal from "../../modal/ControllerHelpModal";

const connectionStatusColor: {
  [key: string]: "success" | "warning" | "danger";
} = {
  [BifrostConnectionStatus.CONNECTED]: "success",
  [BifrostConnectionStatus.CONNECTING]: "warning",
  [BifrostConnectionStatus.DISCONNECTED]: "danger",
};

export const NovaNavbar: React.FC = () => {
  const uiActions = useUIActions();
  // const rosUrl = useSelector((state: RootState) => state.uiState.rosUrl);

  const bifrostStatus = useSelector(
    (state: RootState) => state.bifrostStatus.connectionStatus
  );

  // State to control the visibility of the modal
  const [isControllerHelpModalOpen, setControllerHelpModalOpen] = useState(false);

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
          {/* Separate button for Controller Help */}
          <Button
            radius="sm"
            size="sm"
            onClick={() => setControllerHelpModalOpen(true)}
          >
            Controller Help
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

      {/* Controller Help Modal */}
      <ControllerHelpModal
        isOpen={isControllerHelpModalOpen}
        onClose={() => setControllerHelpModalOpen(false)}
      />
    </Navbar>
  );
};
