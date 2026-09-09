textures/river_outpost/terrain_0
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 1024 1024
	q3map_tcGen ivector ( 192 0 0 ) ( 0 192 0 )
	q3map_foliage models/foliage/grassfoliage1.md3 0.7 16 0.025 0
	q3map_foliage models/foliage/grassfoliage2.md3 0.7 16 0.025 0
	q3map_foliage models/foliage/grassfoliage3.md3 0.7 16 0.025 0
	surfaceparm grasssteps
	{
		map textures/wt_terrain/grass_dense1.png
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}
}

textures/river_outpost/terrain_1
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 1024 1024
	q3map_tcGen ivector ( 192 0 0 ) ( 0 192 0 )
		surfaceparm grasssteps
	{
		map textures/wt_terrain/grass_path1.png
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}
}

textures/river_outpost/terrain_2
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 1024 1024
	q3map_tcGen ivector ( 192 0 0 ) ( 0 192 0 )
		q3map_foliage models/foliage/grassfoliage1.md3 0.7 16 0.025 0
	q3map_foliage models/foliage/grassfoliage2.md3 0.7 16 0.025 0
	q3map_foliage models/foliage/grassfoliage3.md3 0.7 16 0.025 0
	surfaceparm grasssteps
	{
		map textures/wt_terrain/grass_dirt3.png
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}
}

textures/river_outpost/terrain_0to1
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 1024 1024
	q3map_tcGen ivector ( 192 0 0 ) ( 0 192 0 )
		surfaceparm grasssteps
	
	{
		map textures/wt_terrain/grass_dense1.png
	}
	{
		map textures/wt_terrain/grass_path1.png
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}
}

textures/river_outpost/terrain_0to2
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 1024 1024
	q3map_tcGen ivector ( 192 0 0 ) ( 0 192 0 )
		surfaceparm grasssteps
	
	{
		map textures/wt_terrain/grass_dense1.png
	}
	{
		map textures/wt_terrain/grass_dirt3.png
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}
}

textures/river_outpost/terrain_1to2
{
	q3map_lightmapsamplesize 64
	q3map_lightmapaxis z
	q3map_texturesize 1024 1024
	q3map_tcGen ivector ( 192 0 0 ) ( 0 192 0 )
		surfaceparm grasssteps
	
	{
		map textures/wt_terrain/grass_path1.png
	}
	{
		map textures/wt_terrain/grass_dirt3.png
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		tcGen lightmap
	}
}

textures/river_outpost/terrain.vertex
{
	{
		map textures/wt_terrain/grass_dense1.png
		rgbGen vertex
	}
}

// Skybox

textures/outpost/rainy_night
{
	nocompress
	qer_editorimage textures/skies/sky_8.tga
	q3map_lightimage textures/skies/n_blue2.tga
	q3map_nofog
	q3map_globaltexture
	q3map_lightsubdivide 1024 // 96 is ok on release 
	q3map_sun 0.130 0.080 0.020 4 165 5
	q3map_skylight 60 4 // 60 8
	q3map_surfacelight 10

	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	skyparms - 512 -
	sunshader textures/skies_sd/small_moon

	{
		map textures/skies_sd/wurzburg_clouds.tga
		tcMod scale 2.5 2.5
		tcMod scroll 0.002 -0.0015
		rgbGen identityLighting
	}
	{
		map textures/skies_sd/ydnar_lightning.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave triangle -8 9 0 0.137
		//alphaGen wave noise -3 4 0 2.37
		tcMod scale 3 3
		tcMod scroll 0.003 -0.0015
	}
	{
		clampmap textures/skies_sd/wurzburg_fogmask.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod scale 0.956 0.956
		tcMod transform 1 0 0 1 -1 -1
		rgbGen identitylighting
	}
}