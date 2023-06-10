from dash import Dash, html, dcc, callback, Output, Input
from os import listdir
import plotly.express as px
import pandas as pd

df = pd.read_csv('/home/cbach/log.csv')

files = [fn for fn in listdir('/home/cbach') if fn.startswith("log") and fn.endswith(".csv")]
files = list(sorted(files))

columns = [col for col in df.columns.to_list() if col not in ['Date', ' Time']]

app = Dash(__name__)

app.layout = html.Div([
    html.H1(children='Sauna Sensor', style={'textAlign':'center'}),
    dcc.Dropdown(files, 'log.csv', id='dropdown-selection-file'),
    dcc.Dropdown(columns, ' Temperature [°C]', id='dropdown-selection'),
    dcc.Graph(id='graph-content')
])

@callback(
    Output('graph-content', 'figure'),
    Input('dropdown-selection-file', 'value'),
    Input('dropdown-selection', 'value')
)
def update_graph(value, column):
    dff = pd.read_csv(value)
    return px.line(dff, x=' Time', y=column)

if __name__ == '__main__':
    app.run_server(debug=True, host='0.0.0.0')

