import { useState } from 'react';
import { AppSession } from '../interfaces';

import { Chart } from 'chart.js';
import zoomPlugin from 'chartjs-plugin-zoom';

interface LineChartAppDataUsageProps {
  appSessions: AppSession[];
}

function LineChartAppDataUsage(props: LineChartAppDataUsageProps) {
  return <div className="block-chart"></div>;
}

export default LineChartAppDataUsage;
