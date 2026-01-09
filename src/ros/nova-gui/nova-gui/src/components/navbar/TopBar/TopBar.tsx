import {
  Button,
  Divider,
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
import novaLogo from "../../../assets/nova-logo.png";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { BifrostConnectionStatus } from "../../../redux/models/bifrost/BifrostTypes.ts";
import { Link, useLocation } from "react-router-dom";
import humanizeString from "humanize-string";
import { BLCMDStatusButton } from "../BLCMDStatusModal/BLCMDStatusButton.tsx";
import "./TopBar.css";
import { List } from "react-bootstrap-icons";
import { BatteryWidget } from "../BatteryWidget/BatteryWidget.tsx";
import {RGBInputModal} from "../RGBWidget/RGBModal.tsx";
import {RadioStatusButton} from "../RadioStatusModal/RadioStatusButton.tsx";

const connectionStatusColor: {
  [key: string]: "success" | "warning" | "danger";
} = {
  [BifrostConnectionStatus.CONNECTED]: "success",
  [BifrostConnectionStatus.CONNECTING]: "warning",
  [BifrostConnectionStatus.DISCONNECTED]: "danger",
};

const prettyViewNames = new Map<string, string>([
  ["", "Home"],
  ["general", "General"],
  ["arc", "ARC"],
  ["urc", "URC"],
  ["test", "Test"],
  ["cameras", "Cameras"],
]);

export const NovaTopBar: React.FC = () => {
  const uiActions = useUIActions();

  const uiState = useSelector((state: RootState) => state.uiState);

  const bifrostStatus = useSelector(
    (state: RootState) => state.bifrostStatus.connectionStatus
  );

  const location = useLocation();

  const parsedLocation = location.pathname
    .split("/")
    .filter((val) => !["/", ""].includes(val));

  const viewName = parsedLocation.length !== 0 ? parsedLocation[0] : "";
  const title = parsedLocation.reverse()[0];

  return (
    <Navbar maxWidth="full" isBordered position="static">
      <Button
        variant="light"
        isIconOnly
        onPress={() =>
          uiActions.setSideBarVisibility(!uiState.sidebarIsVisible)
        }
        className="absolute left-2"
      >
        <List size="24px" />
      </Button>
      <NavbarContent justify="start" className="ml-7">
        <NavbarBrand>
          <Link to="/">
            <img src={novaLogo} className="w-16" alt="Nova Logo" />
          </Link>
          {!!title && (
            <>
              <Divider orientation="vertical" className="h-10 w-[2px] mx-2" />
              <p className="title hidden sm:block text-2xl ">
                {humanizeString(title)}
              </p>
            </>
          )}
        </NavbarBrand>
      </NavbarContent>
      <NavbarContent as="div" className="items-center" justify="end">
        <NavbarItem>
          <BLCMDStatusButton />
        </NavbarItem>
        <NavbarItem>
            <RadioStatusButton />
        </NavbarItem>
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
              <DropdownItem key={"shut-down"}>Shut Down</DropdownItem>
              <DropdownItem key={"restart"}>Restart</DropdownItem>
              <DropdownItem key={"disconnect"}>Disconnect</DropdownItem>
            </DropdownMenu>
          </Dropdown>
        </NavbarItem>
        <NavbarItem>
          <Dropdown placement="bottom-end" backdrop="blur">
            <DropdownTrigger>
              <Button radius="sm" size="sm">
                <div className="w-10">
                  {prettyViewNames.has(viewName) ? prettyViewNames.get(viewName) : "???"}
                </div>
                <ChevronDown className="w-4 h-4"/>
              </Button>
            </DropdownTrigger>
            <DropdownMenu aria-label="Operation Mode">
              <DropdownItem
                description="General Tab for Rover Operation"
                href="/general"
                key={"general"}
              >
                General
              </DropdownItem>
              <DropdownItem
                description="Australian Rover Challenge"
                href="/arc"
                key={"arc"}
              >
                ARC
              </DropdownItem>
              <DropdownItem
                description="University Rover Challenge"
                href="/urc"
                key={"urc"}
              >
                URC
              </DropdownItem>
              <DropdownItem
                description="Pages for Testing"
                href="/test"
                key={"test"}
              >
                Test
              </DropdownItem>
              <DropdownItem
                description="GUI Home Page"
                href="/"
                key={"home"}
              >
                Home
              </DropdownItem>
            </DropdownMenu>
          </Dropdown>
        </NavbarItem>
        <NavbarItem className="">
          <Button
            radius="sm"
            size="sm"
          >
            <BatteryWidget />
          </Button>
        </NavbarItem>
        <NavbarItem className="">
          <>
            <RGBInputModal />
          </>
        </NavbarItem>
        <NavbarItem>
          {/*Controller Help Modal*/}
          <Button
            isIconOnly
            radius="sm"
            size="sm"
            variant="shadow"
            onPress={() => uiActions.setControllerHelpModal(true)}
          >
            <HelpCircle className="w-4 h-4 " />
          </Button>
        </NavbarItem>
        <NavbarItem>
          <Button
            isIconOnly
            size="sm"
            variant="shadow"
            onPress={() => uiActions.setSettingsModal(true)}
          >
            <Settings className="w-4 h-4" />
          </Button>
        </NavbarItem>
      </NavbarContent>
    </Navbar>
  );
};
