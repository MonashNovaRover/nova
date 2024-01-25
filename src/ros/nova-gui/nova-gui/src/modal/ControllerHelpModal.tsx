import React, { useEffect } from 'react';
import ControllerHelpView from '../views/controllerHelp/ControllerHelpView';


interface ControllerHelpModalProps {
    isOpen: boolean;
    onClose: () => void;
}

const ControllerHelpModal: React.FC<ControllerHelpModalProps> = ({ isOpen, onClose }) => {
    useEffect(() => {
        const handleCloseModal = (event: MouseEvent) => {
            // Check if the click is outside the modal
            const modal = document.getElementById('controller-help-modal');
            if (modal && !modal.contains(event.target as Node)) {
                onClose();
            }
        };

        if (isOpen) {
            // Attach event listener when the modal is open
            document.addEventListener('click', handleCloseModal);
        }

        // Clean up the event listener when the modal is closed or the component unmounts
        return () => {
            document.removeEventListener('click', handleCloseModal);
        };
    }, [isOpen, onClose]);

    return (
        <>
            {isOpen && (
                <div className="modal-overlay" id="controller-help-modal">
                    <div className="modal">
                        <button className="modal-close" onClick={onClose}>
                            Close
                        </button>
                        <ControllerHelpView />
                    </div>
                </div>
            )}
        </>
    );
};

export default ControllerHelpModal;
