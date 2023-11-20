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
import React from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/models/RootState";
import { useUIActions } from "../../redux/actions/useUIActions";
import { useBifrostAction } from "../../redux/actions/useBifrostAction";
import { Ros } from "roslib";

export const NovaNavbar: React.FC = () => {
  const [connected, setConnected] = useState(false);

  const rosUrl = useSelector((state: RootState) => state.uiState.rosUrl);

  const uiActions = useUIActions();

  const bifrostActions = useBifrostAction<IRadioStatus>();

  useEffect(() => {
    const ros = new Ros({ url: rosUrl });

    ros.on("connection", () => {
      setConnected(true);
    });
  }, [rosUrl]);

  return (
    <Navbar maxWidth="full" isBordered position="static">
      <NavbarContent justify="start">
        <NavbarBrand>
          <img src={novaLogo} className="w-14"></img>
        </NavbarBrand>
      </NavbarContent>
      <NavbarContent as="div" className="items-center" justify="end">
        <NavbarItem>
          <Dropdown placement="bottom-end">
            <DropdownTrigger>
              <Button radius="sm" color="success" size="sm" variant="shadow">
                Connected
              </Button>
            </DropdownTrigger>
            <DropdownMenu>
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
          <Button
            isIconOnly
            size="sm"
            variant="shadow"
            onClick={() => bifrostActions.updateStuff({ ping: 200 })}
          >
            <Settings className="w-4 h-4" />
          </Button>
        </NavbarItem>
      </NavbarContent>
    </Navbar>
  );
};
