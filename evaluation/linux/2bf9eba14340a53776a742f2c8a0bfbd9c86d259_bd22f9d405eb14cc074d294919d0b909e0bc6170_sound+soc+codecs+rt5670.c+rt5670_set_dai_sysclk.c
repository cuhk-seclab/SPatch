static int rt5670_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct rt5670_priv *rt5670 = snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val = 0;

	if (freq == rt5670->sysclk && clk_id == rt5670->sysclk_src)
		return 0;

	switch (clk_id) {
	case RT5670_SCLK_S_MCLK:
		reg_val |= RT5670_SCLK_SRC_MCLK;
		break;
	case RT5670_SCLK_S_PLL1:
		reg_val |= RT5670_SCLK_SRC_PLL1;
		break;
	case RT5670_SCLK_S_RCCLK:
		reg_val |= RT5670_SCLK_SRC_RCCLK;
		break;
	default:
		dev_err(codec->dev, "Invalid clock id (%d)\n", clk_id);
		return -EINVAL;
	}
	snd_soc_update_bits(codec, RT5670_GLB_CLK,
		RT5670_SCLK_SRC_MASK, reg_val);
	rt5670->sysclk = freq;
	rt5670->sysclk_src = clk_id;

	dev_dbg(dai->dev, "Sysclk is %dHz and clock id is %d\n", freq, clk_id);

	return 0;
}
