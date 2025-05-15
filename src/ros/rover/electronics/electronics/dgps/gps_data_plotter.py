#!/usr/bin/env python3
import pandas as pd
import folium

# Load the CSV (replace with your filename)
df = pd.read_csv('data-3.csv')  # Columns: 'lat', 'lon'
df = pd.read_csv('data-3.csv', header=None)
df = df.iloc[:, -2:]  # Keep only the last two columns
df.columns = ['lat', 'lon']  # Rename for clarity

# Start map centered around first point
m = folium.Map(location=[df['lat'].mean(), df['lon'].mean()], zoom_start=20)

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
m.save('map-3.html')