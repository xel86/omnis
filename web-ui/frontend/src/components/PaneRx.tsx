import { useState } from 'react';
import { convertToUnit, UNITS } from '../units';

interface PaneRxProps {
  bytesRx: number;
  pktRx: number;
}

function PaneRx(props: PaneRxProps) {
  const [unitIndex, setUnitIndex] = useState(1);

  return (
    <div className="block-tx-rx">
      <div className="rounded-full py-4 px-5 bg-red-200 font-bold text-red-700 xl:text-xl">
        Rx
      </div>

      <div className="w-full flex justify-evenly">
        <div className="flex flex-col text-right justify-center">
          <div className="text-2xl font-bold xl:text-4xl">{props.pktRx}</div>
          <div className="text-sm mt-[-0.5rem] xl:text-md">PACKETS</div>
        </div>

        <div className="flex items-center space-x-2">
          <div className="text-4xl font-bold text-right min-w-[4rem] xl:text-5xl">
            {unitIndex === 0
              ? convertToUnit(props.bytesRx, unitIndex)
              : convertToUnit(props.bytesRx, unitIndex).toFixed(2)}
          </div>
          <button
            className="button-unit"
            onClick={() => setUnitIndex((unitIndex + 1) % UNITS.length)}
          >
            {UNITS[unitIndex]}
          </button>
        </div>
      </div>
    </div>
  );
}

export default PaneRx;
