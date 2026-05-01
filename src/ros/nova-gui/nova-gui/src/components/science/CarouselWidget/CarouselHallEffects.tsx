import {useCarouselFeedback} from "./useCarouselBifrost.ts";
import {Chip} from "@nextui-org/react";

// temp hall effect sensor display
export const CarouselHallEffects = () => {
  const [innerFeedback, outerFeedback] = useCarouselFeedback();

  return (
    <div className="flex flex-col w-full gap-3 mt-3 justify-items-center">
      <Chip
        key={"inner_" + innerFeedback.hall_effect_triggered.toString()}
        radius='md'
        size="lg"
        variant="dot"
        classNames={{
          base: `h-10 border-3 border-${innerFeedback.hall_effect_triggered ? "success" : "danger"}`,
          dot: `bg-${innerFeedback.hall_effect_triggered ? "success" : "danger"}`
        }}
      >
        Inner Carousel {innerFeedback.hall_effect_triggered ? "triggered" : "not triggered"}
      </Chip>
      <Chip
        key={"outer_" + outerFeedback.hall_effect_triggered.toString()}
        radius='md'
        size="lg"
        variant="dot"
        classNames={{
          base: `h-10 border-3 border-${outerFeedback.hall_effect_triggered ? "success" : "danger"}`,
          dot: `bg-${outerFeedback.hall_effect_triggered ? "success" : "danger"}`
        }}
      >
        Outer Carousel {outerFeedback.hall_effect_triggered ? "triggered" : "not triggered"}
      </Chip>
    </div>
  )
}
