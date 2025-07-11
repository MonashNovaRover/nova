import {
  Modal,
  ModalBody,
  ModalContent,
  ModalHeader, Tab, Tabs,
} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import StoreSettings from "./StoreSettings.tsx";
import {IPSettings} from "./IPSettings.tsx";

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
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">Settings</ModalHeader>
        <ModalBody>

          <Tabs
            variant="underlined"
            classNames={{
              tabList: "gap-6 w-full relative rounded-none p-0 border-b border-divider",
            }}
          >
            <Tab title="IP">
              <IPSettings/>
            </Tab>

            <Tab title="Store">
              <StoreSettings/>
            </Tab>
          </Tabs>

        </ModalBody>
      </ModalContent>
    </Modal>
  );
}
