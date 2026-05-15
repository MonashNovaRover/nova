import React, { createContext, useContext, useState, useCallback, ReactNode } from "react";

/**
 * Context for sharing carousel position state between components.
 *
 * This allows the UVVisSpec component to access the current carousel
 * position without prop drilling, enabling dynamic graph naming based
 * on which cuvette is currently selected.
 */
export interface CarouselPositionContextValue {
  innerCuvette: number;  // 0-indexed
  outerCuvette: number;  // 0-indexed
  setPositions: (inner: number, outer: number) => void;
}

const CarouselPositionContext = createContext<CarouselPositionContextValue | undefined>(undefined);

export interface CarouselPositionProviderProps {
  children: ReactNode;
}

export const CarouselPositionProvider: React.FC<CarouselPositionProviderProps> = ({ children }) => {
  const [innerCuvette, setInnerCuvette] = useState(0);
  const [outerCuvette, setOuterCuvette] = useState(0);

  const setPositions = useCallback((inner: number, outer: number) => {
    setInnerCuvette(inner);
    setOuterCuvette(outer);
  }, []);

  return (
    <CarouselPositionContext.Provider value={{ innerCuvette, outerCuvette, setPositions }}>
      {children}
    </CarouselPositionContext.Provider>
  );
};

/**
 * Hook to access carousel position context.
 * Returns undefined if no provider is present (graceful fallback).
 */
export function useCarouselPosition(): CarouselPositionContextValue | undefined {
  return useContext(CarouselPositionContext);
}
