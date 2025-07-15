import {Button, Image, Modal, ModalBody, ModalContent, ModalFooter, ModalHeader,} from "@nextui-org/react";
import {useEffect, useState} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import BanksiaAuto from "../../../../assets/banksia-auto.png"
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";
import {IRosNovaInterfacesStatusConst} from "../../../../ros/rosTypes.ts";

interface AutoArrivedPopupProps {
}

export const AutoArrivedPopup : React.FC<AutoArrivedPopupProps> = () => {
  const bifrost = useBifrost({topic: RosTopic.AUTO_STATUS})
  const autoStatus = useSelector((state: RootState) => state.autoStatus.status)
  const roverLocation = useSelector((state: RootState) => state.roverLocationStore)
  const [isOpen, setIsOpen] = useState(false)
  const [arrivedLocation, setArrivedLocation] = useState(roverLocation)
  const [lastStatus, setLastStatus] = useState<IRosNovaInterfacesStatusConst>(IRosNovaInterfacesStatusConst.IDLE)

  useEffect(() => {
    bifrost.syncWithTopic()
  }, [bifrost]);

  useEffect(() => {
    if (autoStatus === IRosNovaInterfacesStatusConst.ARRIVED_SUCCESSFULLY && lastStatus != autoStatus) {
      setArrivedLocation(roverLocation)
      setIsOpen(true)
    }
    setLastStatus(autoStatus)
  }, [autoStatus]);

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
            {`(${arrivedLocation.latitude}, ${arrivedLocation.longitude})`}
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
