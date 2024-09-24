import LSObjectCard from "./LSObjectCard.tsx";

export default function TestStateView () {

  return (
    <div className="grid grid-rows-3 gap-4 py-4 px-4">
      <div className="grid grid-cols-2 gap-4">
        <LSObjectCard/>
        <LSObjectCard/>
      </div>
    </div>
  );
}