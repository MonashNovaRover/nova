import { Modal } from "@nextui-org/react";

interface SidebarProps {
  isOpen: boolean;
  onClose: () => void;
  children: React.ReactNode;
}

export const SidebarWrapper = (props: SidebarProps) => {
  return (
    <Modal
      radius="lg"
      size="md"
      backdrop="opaque"
      isOpen={props.isOpen}
      onClose={props.onClose}
      scrollBehavior="inside"
      shadow="lg"
      classNames={{
        wrapper: [
          "fixed",
          "top-0",
          "left-0",
          "bottom-0",
          "items-start",
          "sm:items-start",
          "[--scale-enter:100%]",
          "[--scale-exit:100%]",
          "[--opacity-enter:100%]",
          "[--opacity-exit:0%]",
          "[--slide-y-enter:0px]",
          "[--slide-y-exit:0px]",
          "[--slide-x-enter:0px]",
          "[--slide-x-exit:-200px]",
          "sm:[--scale-enter:100%]",
          "sm:[--scale-exit:100%]",
          "justify-start",
          "w-[300px]",
        ],
        base: [
          "h-full max-h-full",
          "m-0",
          "sm:m-0",
          "dark",
          "text-foreground",
          "rounded-l-none",
        ],
        closeButton: ["right-3", "top-3"],
        header: ["pr-12"],
      }}
      motionProps={{
        variants: {
          enter: {
            x: "var(--slide-x-enter)",
            opacity: "var(--opacity-enter)",
            scale: "var(--scale-enter)",
            y: "var(--slide-y-enter)",
            transition: {
              duration: 0.2,
              ease: "easeIn",
            },
          },
          exit: {
            x: "var(--slide-x-exit)",
            opacity: "var(--opacity-exit)",
            scale: "var(--scale-exit)",
            y: "var(--slide-y-exit)",
            transition: {
              duration: 0.1,
              ease: "easeOut",
            },
          },
        },
      }}
    >
      {props.children}
    </Modal>
  );
};
