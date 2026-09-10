import {useState} from "react";
import {
    Button,
    Modal,
    ModalBody,
    ModalDialog,
    ModalHeader,
} from "@heroui/react";
import RGBInput from "./RGBInput.tsx";

/**
 * Modal for RGB Input
 */
export function RGBInputModal() {
    const [isOpen, setIsOpen] = useState(false);

    const onClose = () => {
        setIsOpen(false);
    };

    const openModal = () => {
        setIsOpen(true);
    };

    return (
        <>
        <Button size="sm" onPress={openModal}>
            Colour
        </Button>
        <Modal className="dark text-foreground" isOpen={isOpen} onClose={onClose}>
            <ModalDialog>
                <ModalHeader className="flex flex-col gap-1">RGB Color Input</ModalHeader>
                <ModalBody>
                    <RGBInput />
                </ModalBody>
            </ModalDialog>
        </Modal>
        </>
    );
}
