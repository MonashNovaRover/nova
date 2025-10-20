import { useState } from "react";
import { Button } from "@nextui-org/react";
import { CartographerGoalModal } from "./CartographerGoalModal.tsx";

export const GoalPublisher = () => {
  const [modalOpen, setModalOpen] = useState(false);

  return (
    <>
      <Button
        variant="shadow"
        fullWidth
        onClick={() => setModalOpen(true)}
      >
        Publish Goals
      </Button>
      <CartographerGoalModal
        isOpen={modalOpen}
        onClose={() => setModalOpen(false)}
      />
    </>
  );
};