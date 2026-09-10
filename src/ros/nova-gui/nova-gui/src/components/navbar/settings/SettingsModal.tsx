import {
  Modal, ModalBody, ModalDialog, ModalHeader, Tab, TabList, TabPanel, Tabs,
} from "@heroui/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import StoreSettings from "./StoreSettings.tsx";
import {IPSettings} from "./IPSettings.tsx";
import { YoloSettings } from "./YoloSettings.tsx";

/**
 * Settings Model containing GUI settings
 * @constructor
 */
export function SettingsModal() {
  const uiActions = useUIActions();
  const uiState = useSelector((state: RootState) => state.uiState);

  const closeModal = () => uiActions.setSettingsModal(false);
  return (
    <Modal
      className="dark text-foreground"
      isOpen={uiState.settingsModalOpen}
      onClose={closeModal}
    >
      <ModalDialog>
        <ModalHeader className="flex flex-col gap-1">Settings</ModalHeader>
        <ModalBody>

          <Tabs
            variant="underlined"
          >
            <TabList className="gap-6 w-full relative rounded-none p-0 border-b border-divider">
              <Tab id="ip">IP</Tab>
              <Tab id="store">Store</Tab>
              <Tab id="yolo">YOLO</Tab>
            </TabList>
            <TabPanel id="ip">
              <IPSettings/>
            </TabPanel>
            <TabPanel id="store">
              <StoreSettings/>
            </TabPanel>
            <TabPanel id="yolo">
              <YoloSettings />
            </TabPanel>
          </Tabs>

        </ModalBody>
      </ModalDialog>
    </Modal>
  );
}
