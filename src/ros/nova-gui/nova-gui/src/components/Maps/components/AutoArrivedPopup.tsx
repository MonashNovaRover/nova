import {
  Modal,
  ModalContent,
  ModalHeader,
  ModalBody,
  ModalFooter,
  Button, Image,
} from "@nextui-org/react";
import {useState} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import BanksiaAuto from "../../../assets/banksia-auto.png"

interface AutoArrivedPopupProps {
}

export const AutoArrivedPopup : React.FC<AutoArrivedPopupProps> = () => {
  const roverLocation = useSelector((state: RootState) => state.roverLocationStore)
  const [isOpen, setIsOpen] = useState(true)

  const onClose = () => setIsOpen(false)

  return (
    <Modal isOpen={isOpen} size="5xl" onClose={onClose} className="dark text-foreground"  classNames={{
      base: "bg-success-100 text-[#a8b0d3]",
    }}>
      <ModalContent>
        <ModalHeader className="flex justify-center gap-1 text-6xl mt-5">
          <span>Banksia Has Arrived</span>
        </ModalHeader>
        <ModalBody className="flex flex-col items-center text-center gap-3">
          <p>
            at:
          </p>
          <p className="text-4xl">
            {`(${roverLocation.latitude}, ${roverLocation.longitude})`}
          </p>
          <Image src={BanksiaAuto} removeWrapper className="w-1/2 mt-10 items-center"/>
        </ModalBody>
        <ModalFooter>
          <Button color="danger" variant="ghost" onPress={onClose}>
            Close
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  )
}

export default AutoArrivedPopup;
