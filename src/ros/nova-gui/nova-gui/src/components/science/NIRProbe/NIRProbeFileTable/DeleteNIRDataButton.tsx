import {Button, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader} from "@nextui-org/react";
import React, {useState} from "react";
import {useNIRSiteData} from "../useNIRSiteData.ts";
import {Trash2} from "react-feather";
import {NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";

/**
 * Table containing the average reading and concentration
 * @constructor
 */
const DeleteNIRDataButton: React.FC = () => {
  const [_, setReadings] = useNIRSiteData();
  const [showModel, setShowModel] = useState(false);

  const deleteData = () => {
    setReadings({
      [NIRProbeReadingType.PD1]: [],
      [NIRProbeReadingType.PD2]: [],
    })
    setShowModel(false)
  }

  const modal = (
    <Modal
      size="2xl"
      className="dark text-foreground"
      isOpen={showModel}
      onClose={() => setShowModel(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">
          Delete all NIR data?
        </ModalHeader>
        <ModalBody>
          <span>This will delete all NIR data stored on this computer, this action cannot be undone.</span>
        </ModalBody>
        <ModalFooter>
          <Button variant="light" onPressStart={() => setShowModel(false)}>
            Close
          </Button>
          <Button color="danger" onPressStart={deleteData}>
            Delete
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  )

  return (
    <div>
      <Button
        isIconOnly
        color="danger"
        variant="light"
        onPressStart={() => setShowModel(true)}
      >
        <Trash2/>
      </Button>

      {modal}
    </div>
  );
}

export default DeleteNIRDataButton;