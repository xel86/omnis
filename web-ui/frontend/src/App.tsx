import { Input } from 'reactstrap';
import { useCallback, useEffect, useState } from 'react';
import { MdApps, MdDarkMode, MdLightMode } from 'react-icons/md';
import { IoMdClose } from 'react-icons/io';
import { Chart } from 'chart.js';
import { Application, AppSession, SessionTotal } from './interfaces';
import PaneRx from './components/PaneRx';
import PaneTx from './components/PaneTx';
import LineChartAppDataUsage from './components/LineChartAppDataUsage';
import BarChartAppDataUsagePerInterval from './components/BarChartAppDataUsageInterval';
import PieChartAppDataUsage from './components/PieChartAppDataUsage';
import PieChartAppPackets from './components/PieChartAppPackets';
import LineChartAppTcp from './components/LineChartAppTcp';
import LineChartAppUdp from './components/LineChartAppUdp';
import PieChartAppTcpUdpPackets from './components/PieChartAppTcpUdpPackets';

const MS_MINUTE = 60000;

function randomHexColor(): string {
  return '#' + ((Math.random() * 0xffffff) << 0).toString(16).padStart(6, '0');
}

function getDateTimeString(date: Date): string {
  const offset = date.getTimezoneOffset();
  date = new Date(date.getTime() - offset * 60 * 1000);
  return date.toISOString().substring(0, 16);
}

async function getData(
  start: Date,
  end: Date
): Promise<AppSession[] | undefined> {
  const response: Response = await fetch(
    (process.env.REACT_APP_API_BASE_URL ?? 'http://localhost:29687') +
      '/data?' +
      new URLSearchParams({
        start: start.toISOString(),
        end: end.toISOString(),
      })
  ).catch((err: Error) => {
    throw err;
  });

  const body = await response.json();
  const appSessions: AppSession[] = body.data;
  if (response.ok) return appSessions;

  return Promise.resolve(undefined);
}

async function getApplications(): Promise<Application[] | undefined> {
  const response: Response = await fetch(
    (process.env.REACT_APP_API_BASE_URL ?? 'http://localhost:29687') +
      '/applications'
  ).catch((err: Error) => {
    throw err;
  });

  const body = await response.json();
  const applications: Application[] = body.data;
  if (response.ok) return applications;

  return Promise.resolve(undefined);
}

async function updateAppColor(app: Application) {
  const requestOptions = {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(app),
  };

  const response: Response = await fetch(
    (process.env.REACT_APP_API_BASE_URL ?? 'http://localhost:29687') +
      '/applications',
    requestOptions
  ).catch((err: Error) => {
    throw err;
  });

  if (!response.ok) alert('Failed to update app color.');
}

function App() {
  const [render, rerender] = useState(false);
  const [end, setEnd] = useState(new Date());
  const [start, setStart] = useState(new Date(end.getTime() - 5 * MS_MINUTE));
  const [isToPresent, setToPresent] = useState(false);
  const [isAppPaneOpen, setIsAppPaneOpen] = useState(false);
  const [apps, setApps] = useState([] as Application[]);
  const [selectedApps, setSelectedApps] = useState([] as Application[]);
  const [appSessions, setAppSessions] = useState([] as AppSession[]);
  const [sessionTotal, setSessionTotal] = useState({
    bytesTx: 0,
    bytesRx: 0,
    pktTx: 0,
    pktRx: 0,
    pktTcp: 0,
    pktUdp: 0,
  } as SessionTotal);
  const [appColor, setAppColor] = useState(
    [] as { name: string; colorHex: string }[]
  );

  useEffect(() => {
    const getApplicationsIntervalId = setInterval(
      async () => {
        const apps = await getApplications();
        if (apps) setApps(apps);
      },
      apps.length > 0 ? 10000 : 1000
    );

    return () => clearInterval(getApplicationsIntervalId);
  });

  useEffect(() => {
    const getDataIntervalId = setInterval(async () => {
      if (isToPresent) {
        setEnd(new Date());
        updateData();
      }
    }, 5000);

    return () => clearInterval(getDataIntervalId);
  }, [start, end, isToPresent, selectedApps]);

  const updateData = useCallback(async () => {
    const appSessions = await getData(start, end);
    if (appSessions) {
      appSessions.forEach((as, idx) => {
        if (as.application.colorHex.length === 0) {
          let color = appColor.find(
            (ac) => ac.name === as.application.name
          )?.colorHex;

          if (!color) {
            color = randomHexColor();
            const arr = appColor;
            arr.push({ name: as.application.name, colorHex: color });
            setAppColor(arr);
          }

          appSessions[idx].application.colorHex = color;
        }
      });

      setAppSessions(appSessions);

      if (selectedApps.length === 0) {
        setSelectedApps(appSessions?.map((as) => as.application));
      }
    }
  }, [start, end]);

  let selectedAppSessions = appSessions.filter((as) =>
    selectedApps.find((a) => as.application.id === a.id)
  );

  useEffect(() => {
    if (appSessions) {
      const sessionTotal: SessionTotal = {
        bytesTx: 0,
        bytesRx: 0,
        pktTx: 0,
        pktRx: 0,
        pktTcp: 0,
        pktUdp: 0,
      };

      appSessions.forEach((appSess) => {
        if (selectedApps.find((a) => appSess.application.id === a.id))
          appSess.sessions.forEach((s) => {
            sessionTotal.bytesTx += s.bytesTx;
            sessionTotal.bytesRx += s.bytesRx;
            sessionTotal.pktTx += s.pktTx;
            sessionTotal.pktRx += s.pktRx;
            sessionTotal.pktTcp += s.pktTcp;
            sessionTotal.pktUdp += s.pktUdp;
          });
      });

      setSessionTotal(sessionTotal);
    }
  }, [selectedApps, appSessions]);

  const isDarkMode =
    localStorage.getItem('theme') === 'dark' ||
    (!('theme' in localStorage) &&
      window.matchMedia('(prefers-color-scheme: dark)').matches);

  if (isDarkMode) {
    document.documentElement.classList.add('dark');
    Chart.defaults.color = '#fff';
  } else {
    document.documentElement.classList.remove('dark');
    Chart.defaults.color = '#666';
  }

  return (
    <>
      <div className="h-screen p-4 bg-gray-100 dark:bg-black">
        <div className="content-pane">
          <div className="w-full p-4 flex">
            <Input
              className="input-datetime"
              type="datetime-local"
              name="dateFrom"
              id="dateFrom"
              placeholder="datetime placeholder"
              value={getDateTimeString(start)}
              onChange={(e) => {
                setStart(new Date(e.target.value));
              }}
              max={getDateTimeString(end)}
            />
            <span className="flex items-center mx-4 text-xl font-semibold dark:text-white">
              -
            </span>
            <Input
              className="input-datetime"
              type="datetime-local"
              name="dateFrom"
              id="dateFrom"
              placeholder="datetime placeholder"
              value={getDateTimeString(end)}
              onChange={(e) => setEnd(new Date(e.target.value))}
              min={getDateTimeString(start)}
              disabled={isToPresent}
            />
            <button
              className={
                'text-sm min-w-max ml-2 px-4 rounded-lg font-semibold transition-all shadow-md ' +
                'dark:text-white ' +
                (isToPresent
                  ? 'bg-blue-600 text-white dark:bg-gray-600 dark:text-white'
                  : 'text-blue-600 hover:bg-gray-100 hover:text-blue-600 dark:hover:text-white dark:hover:bg-gray-600')
              }
              onClick={() => {
                setToPresent(!isToPresent);
                setEnd(new Date());
              }}
            >
              TO PRESENT
            </button>
            <button
              className="button-primary ml-4 font-semibold"
              onClick={updateData}
            >
              APPLY
            </button>

            <div className="w-full flex flex-row-reverse pl-4">
              <button
                className="ml-4 p-2"
                onClick={() => {
                  isDarkMode
                    ? localStorage.setItem('theme', 'light')
                    : localStorage.setItem('theme', 'dark');
                  rerender(!render);
                }}
              >
                {isDarkMode ? (
                  <MdLightMode size={25} className="text-white" />
                ) : (
                  <MdDarkMode size={25} />
                )}
              </button>

              <button
                className="button-primary font-bold"
                onClick={() => setIsAppPaneOpen(!isAppPaneOpen)}
              >
                <MdApps className="mr-1" size={20} /> 20 APPS
              </button>
            </div>
          </div>

          <div className="content-grid">
            <div className="col-span-2">
              <PaneTx
                bytesTx={sessionTotal.bytesTx}
                pktTx={sessionTotal.pktTx}
              />
            </div>
            <div className="col-span-2">
              <PaneRx
                bytesRx={sessionTotal.bytesRx}
                pktRx={sessionTotal.pktRx}
              />
            </div>

            <div className="col-span-4">
              <LineChartAppDataUsage
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>

            <div className="col-span-4">
              <BarChartAppDataUsagePerInterval
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>

            <div className="col-span-2">
              <PieChartAppDataUsage
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>
            <div className="col-span-2">
              <PieChartAppPackets
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>

            <div className="col-span-2">
              <LineChartAppTcp
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>
            <div className="col-span-2">
              <LineChartAppUdp
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>

            <div className="col-span-2">
              <PieChartAppTcpUdpPackets
                appSessions={selectedAppSessions}
                start={start}
                end={end}
                isDarkMode={isDarkMode}
              />
            </div>
          </div>
        </div>
      </div>

      {isAppPaneOpen ? <div className="overlay"></div> : ''}
      {isAppPaneOpen ? (
        <div className="absolute inset-0 h-full w-full flex justify-center items-center">
          <div className="app-pane">
            <button
              className="absolute right-6"
              onClick={() => setIsAppPaneOpen(false)}
            >
              <IoMdClose />
            </button>
            <div className="font-bold text-lg xl:text-xl">
              Set Application Colors / Select Applications to Display
            </div>
            <hr className="border-black dark:border-white" />
            <div className="app-grid">
              {apps.map((a) => (
                <div
                  className={
                    'app-grid-item ' +
                    (selectedApps.find((app) => app.id === a.id)
                      ? 'bg-blue-600 text-white dark:bg-gray-800'
                      : 'hover:border-blue-600 dark:hover:border-gray-800')
                  }
                  key={a.name}
                  onClick={() => {
                    const tmp = [...selectedApps];
                    let idx = 0;
                    if (
                      tmp.find((app, i) => {
                        if (app.id === a.id) {
                          idx = i;
                          return true;
                        }
                      })
                    ) {
                      tmp.splice(idx, 1);
                    } else tmp.push(a);

                    return setSelectedApps(tmp);
                  }}
                >
                  <div>{a.name}</div>
                  <input
                    className=""
                    type="color"
                    value={a.colorHex.length > 0 ? a.colorHex : '#ffffff'}
                    onChange={async (e) => {
                      await updateAppColor({
                        id: a.id,
                        name: a.name,
                        colorHex: e.target.value,
                      });
                      updateData();
                    }}
                  ></input>
                </div>
              ))}
            </div>
          </div>
        </div>
      ) : (
        ''
      )}
    </>
  );
}

export default App;
