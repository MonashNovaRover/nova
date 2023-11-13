import {
  Card,
  CardBody,
  Modal,
  ModalBody,
  ModalContent,
  ModalHeader,
} from "@nextui-org/react";
import { Link } from "react-router-dom";

export const NullView = () => {
  return (
    <div className="dark text-foreground  w-screen h-screen [background:radial-gradient(125%_125%_at_50%_10%,#000_40%,#63e_100%)]">
      <Modal isOpen hideCloseButton shadow="lg">
        <ModalContent className="dark text-foreground ">
          <ModalHeader className="flex flex-col gap-1">Mode</ModalHeader>
          <ModalBody>
            <div className="flex flex-row w-full h-24">
              <Link to={"/"} className="flex-1 m-2">
                <Card isPressable shadow="sm" className="w-full h-full">
                  <CardBody>Base</CardBody>
                </Card>
              </Link>
              <Card isPressable shadow="sm" className="flex-1 m-2">
                <CardBody>ARC</CardBody>
              </Card>
              <Card isPressable shadow="sm" className="flex-1 m-2">
                <CardBody>URC</CardBody>
              </Card>
            </div>
          </ModalBody>
        </ModalContent>
      </Modal>
    </div>
  );
};
