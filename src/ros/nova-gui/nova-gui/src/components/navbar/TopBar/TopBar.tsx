import {
  Button,
  Separator,
  Dropdown,
  DropdownItem,
  DropdownMenu,
  DropdownTrigger,
} from "@heroui/react";
import { ChevronDown, Settings, HelpCircle } from "react-feather";
import novaLogo from "../../../assets/nova-logo.png";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { BifrostConnectionStatus } from "../../../redux/models/bifrost/BifrostTypes.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { Link, useLocation } from "react-router-dom";
import humanizeString from "humanize-string";
import { BLCMDStatusButton } from "../BLCMDStatusModal/BLCMDStatusButton.tsx";
import "./TopBar.css";
import { List } from "react-bootstrap-icons";
import { BatteryWidget } from "../BatteryWidget/BatteryWidget.tsx";
import {RGBInputModal} from "../RGBWidget/RGBModal.tsx";
import {RadioStatusButton} from "../RadioStatusModal/RadioStatusButton.tsx";

const connectionStatusClass: {
  [key: string]: string;
} = {
  [BifrostConnectionStatus.CONNECTED]: "bg-success",
  [BifrostConnectionStatus.CONNECTING]: "bg-warning",
  [BifrostConnectionStatus.DISCONNECTED]: "bg-danger",
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
  const bifrostActions = useBifrost({ topic: RosTopic.NULL_TOPIC });

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
    <header className="flex w-full items-center border-b border-default-200 px-4 py-2">
      <Button
        variant="ghost"
        isIconOnly
        onPress={() =>
          uiActions.setSideBarVisibility(!uiState.sidebarIsVisible)
        }
        className="absolute left-2"
      >
        <List size="24px" />
      </Button>
      <div className="ml-7 flex items-center">
        <div className="flex items-center">
          <Link to="/">
            <img src={novaLogo} className="w-16" alt="Nova Logo" />
          </Link>
          {!!title && (
            <>
              <Separator orientation="vertical" className="h-10 w-[2px] mx-2" />
              <p className="title hidden sm:block text-2xl ">
                {humanizeString(title)}
              </p>
            </>
          )}
        </div>
      </div>
      <div className="ml-auto flex items-center gap-2">
        <div>
          <BLCMDStatusButton />
        </div>
        <div>
            <RadioStatusButton />
        </div>
        <div>
          <Dropdown>
            <DropdownTrigger>
              <Button
                size="sm"
                variant="primary"
                className={connectionStatusClass[bifrostStatus]}
              >
                {bifrostStatus.toString()}
              </Button>
            </DropdownTrigger>
            <Dropdown.Popover placement="bottom end">
              <DropdownMenu
                aria-label="ROS Connection"
                onAction={(key) => {
                  if (key === "reconnect") {
                    bifrostActions.updateBifrostConnection(BifrostConnectionStatus.DISCONNECTED);
                  }
                }}
              >
                <DropdownItem id="reconnect">Reconnect</DropdownItem>
                <DropdownItem id="disconnect">Disconnect</DropdownItem>
              </DropdownMenu>
            </Dropdown.Popover>
          </Dropdown>
        </div>
        <div>
          <Dropdown>
            <DropdownTrigger>
              <Button size="sm">
                <div className="w-10">
                  {prettyViewNames.has(viewName) ? prettyViewNames.get(viewName) : "???"}
                </div>
                <ChevronDown className="w-4 h-4"/>
              </Button>
            </DropdownTrigger>
            <Dropdown.Popover placement="bottom end">
            <DropdownMenu aria-label="Operation Mode">
              <DropdownItem
                href="/general"
                id="general"
              >
                General
              </DropdownItem>
              <DropdownItem
                href="/arc"
                id="arc"
              >
                ARC
              </DropdownItem>
              <DropdownItem
                href="/urc"
                id="urc"
              >
                URC
              </DropdownItem>
              <DropdownItem
                href="/test"
                id="test"
              >
                Test
              </DropdownItem>
              <DropdownItem
                href="/"
                id="home"
              >
                Home
              </DropdownItem>
            </DropdownMenu>
            </Dropdown.Popover>
          </Dropdown>
        </div>
        <div>
          <Button size="sm">
            <BatteryWidget />
          </Button>
        </div>
        <div>
          <>
            <RGBInputModal />
          </>
        </div>
        <div>
          {/*Controller Help Modal*/}
          <Button
            isIconOnly
            size="sm"
            variant="primary"
            onPress={() => uiActions.setControllerHelpModal(true)}
          >
            <HelpCircle className="w-4 h-4 " />
          </Button>
        </div>
        <div>
          <Button
            isIconOnly
            size="sm"
            variant="primary"
            onPress={() => uiActions.setSettingsModal(true)}
          >
            <Settings className="w-4 h-4" />
          </Button>
        </div>
      </div>
    </header>
  );
};
