import React, {useState} from "react";
import {
    Button,
    Modal,
    ModalBody,
    ModalContent,
    ModalHeader,
} from "@nextui-org/react";
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
        <Button onPress={openModal}>
            Color
        </Button>
        <Modal className="dark text-foreground" isOpen={isOpen} onClose={onClose}>
            <ModalContent>
                <ModalHeader className="flex flex-col gap-1">RGB Color Input</ModalHeader>
                <ModalBody>
                    <RGBInput />
                </ModalBody>
            </ModalContent>
        </Modal>
        </>
    );
}
