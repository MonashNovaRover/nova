import React from "react";
import CarouselWidget from "../../../components/CarouselWidget/CarouselWidget.tsx";

const TestCarouselView: React.FC = () => {
  return (
    <div className="grid auto-cols-fr grid-cols-2 p-3">
      <CarouselWidget></CarouselWidget>

    </div>
  );
};

export default TestCarouselView;
