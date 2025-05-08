#!/usr/bin/env python3
import pandas as pd
import folium

# Load the CSV (replace with your filename)
df = pd.read_csv("data.csv", header=None)
df = df.iloc[:, [3, 4]]  # Explicitly select columns 1 and 2 (2nd and 3rd)
df.columns = ['lat', 'lon']  # Rename for clarity

# Start map centered around first point
m = folium.Map(
    location=[df['lat'].mean(), df['lon'].mean()],
    zoom_start=20,
    max_zoom=25,
    tiles=None
)

# Add Esri satellite
folium.TileLayer(
    tiles='https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}',
    name='Satellite',
    attr='Tiles © Esri',
    max_zoom=20
).add_to(m)

# Add CartoDB or OpenStreetMap vector tiles for higher zoom
folium.TileLayer(
    tiles='https://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}{r}.png',
    attr='© OpenStreetMap contributors © CARTO',
    name='CartoDB Light',
    max_zoom=25
).add_to(m)

# Add layer control
folium.LayerControl().add_to(m)

# Add all points
for _, row in df.iterrows():
    folium.CircleMarker(
        location=[row['lat'], row['lon']],
        radius=1,
        color='blue',
        fill=True,
        fill_opacity=0.6
    ).add_to(m)

# Save map
m.save("map.html")