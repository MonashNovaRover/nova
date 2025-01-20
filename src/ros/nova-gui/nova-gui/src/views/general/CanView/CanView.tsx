import CanDumpWidget from "../../../components/CAN/CanDumpWidget/CanDumpWidget.tsx";

/**
 * A webgl hooks stress test, that ensures that some everything is working smoothly.
 * @constructor
 */
export default function CanView() {


  return (
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">

      <CanDumpWidget/>


    </div>
  );
}