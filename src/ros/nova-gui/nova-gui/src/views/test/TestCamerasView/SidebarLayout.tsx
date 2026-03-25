import React from "react";
import { Button } from "@nextui-org/react";
import {Menu} from "react-feather";

interface Props {
  sidebar: React.ReactNode;
  children: React.ReactNode;
  defaultOpen?: boolean;
  expandedWidth?: number;
}

export default function SidebarLayout({
                                        sidebar,
                                        children,
                                        defaultOpen = true,
                                        expandedWidth = 260,
                                      }: Props) {
  const [isOpen, setIsOpen] =
    React.useState(defaultOpen);

  return (
    <div className="flex flex-1 h-full min-h-0">

      {/* Sidebar */}
      <aside
        style={{
          width: isOpen ? expandedWidth : 0,
        }}
        className="
          transition-[width]
          duration-300
          overflow-hidden
          flex flex-col
          h-full
        "
      >
        <div
          className="
            h-full
            bg-content1
            shadow-small
            rounded-r-2xl
            flex flex-col
          "
        >
          {/* Close button */}
          <div className="p-2">
            <Button
              size="sm"
              variant="light"
              onPress={() =>
                setIsOpen(false)
              }
              isIconOnly
            >
              <Menu/>
            </Button>
          </div>

          {/* Sidebar content */}
          <div className="flex-1 overflow-auto">
            {sidebar}
          </div>
        </div>
      </aside>

      {/* Main content */}
      <main
        className="
          flex-1
          overflow-auto
          relative
          transition-all
          duration-300
          min-w-0
        "
      >
        {!isOpen && (
          <div className="absolute top-3 left-3 z-10">
            <Button
              size="sm"
              variant="light"
              onPress={() =>
                setIsOpen(true)
              }
            >
              ⮞
            </Button>
          </div>
        )}

        {children}
      </main>

    </div>
  );
}