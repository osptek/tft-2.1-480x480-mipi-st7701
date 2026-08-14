/*
 * ST7701 480x480 2-Lane MIPI-DSI Panel Driver for Raspberry Pi 5
 * Adapted from CNflysky's panel-rpi-dsi-display.c style
 */

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <video/mipi_display.h>

struct power_on_timing {
	unsigned long post_reset;
	unsigned long reset_low;
	unsigned long after_reset;
	unsigned long slpout;
};

struct st7701_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long flags;
	enum mipi_dsi_pixel_format format;
	int (*init_sequence)(struct mipi_dsi_device *dsi);
	const struct power_on_timing *pwr_timing;
	bool do_sw_reset;
};

struct st7701 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	const struct st7701_desc *desc;
	struct gpio_desc *reset;
	enum drm_panel_orientation orientation;
};

static inline struct st7701 *to_st7701(struct drm_panel *panel)
{
	return container_of(panel, struct st7701, panel);
}

/* ==================== 你的初始化序列 ==================== */
static int st7701_480x480_init_sequence(struct mipi_dsi_device *dsi)
{
	struct mipi_dsi_multi_context ctx = { .dsi = dsi };

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xEF, 0x08);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x3A, 0x77);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x10);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC0, 0x3B, 0x00);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC1, 0x09, 0x05);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC2, 0x07, 0x02);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC6, 0x21);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xCC, 0x30);

	/* Positive Gamma */
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB0, 0x40, 0x87, 0x92, 0x0C, 0x90, 0x07, 0x05, 0x09,
				     0x08, 0x21, 0x06, 0x55, 0x12, 0x25, 0xA8, 0x4F);

	/* Negative Gamma */
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB1, 0xC0, 0x53, 0xD9, 0x0F, 0x12, 0x05, 0x07, 0x08,
				     0x07, 0x23, 0x08, 0x17, 0x15, 0xA3, 0xA6, 0xD6);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB0, 0x6D);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB1, 0x28);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB2, 0x87);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB3, 0x80);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB5, 0x45);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB7, 0x87);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB8, 0x33);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xB9, 0x10);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xBB, 0x03);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC0, 0x03);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC1, 0x78);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xC2, 0x78);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xD0, 0x88);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x11);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE0, 0x00, 0x18, 0x00, 0x00, 0x00, 0x20);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE1, 0x05, 0xA0, 0x00, 0xA0, 0x04, 0x0A, 0x00, 0xA0, 0x00, 0x44, 0x44);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE2, 0x11, 0x11, 0x44, 0x44, 0xEA, 0xA0, 0x00, 0x00, 0xE9, 0xA0, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE3, 0x00, 0x00, 0x11, 0x11);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE4, 0x44, 0x44);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE5, 0x06, 0xE5, 0xD8, 0xA0, 0x08, 0xE7, 0xD8, 0xA0,
				     0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE6, 0x00, 0x00, 0x11, 0x11);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE7, 0x44, 0x44);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE8, 0x05, 0xE4, 0xD8, 0xA0, 0x07, 0xE6, 0xD8, 0xA0,
				     0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xEB, 0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x10);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xEC, 0x3D, 0x02, 0x00);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xED, 0x20, 0x76, 0x54, 0x98, 0xBA, 0xFF, 0xFF, 0xFF,
				     0xFF, 0xFF, 0xFF, 0xAB, 0x89, 0x45, 0x67, 0x02);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xEF, 0x08, 0x08, 0x08, 0x45, 0x1F, 0x54);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x13);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE8, 0x00, 0x0E);

	/* Sleep Out */
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x11);
	msleep(120);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE8, 0x00, 0x0C);
	msleep(10);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE8, 0x00, 0x00);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xE6, 0x16, 0x7C);

	/* 退出 Command2 */
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xFF, 0x77, 0x01, 0x00, 0x00, 0x00);

	/* Display On */
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x29);
	msleep(120);

	mipi_dsi_dcs_write_seq_multi(&ctx, 0x36, 0x00);

	return ctx.accum_err;
}
/* ======================================================= */

static int st7701_prepare(struct drm_panel *panel)
{
	struct st7701 *st7701 = to_st7701(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = st7701->dsi };

	if (st7701->reset) {
		gpiod_set_value_cansleep(st7701->reset, 1);
		msleep(st7701->desc->pwr_timing->post_reset);
		gpiod_set_value_cansleep(st7701->reset, 0);
		msleep(st7701->desc->pwr_timing->reset_low);
		gpiod_set_value_cansleep(st7701->reset, 1);
		msleep(st7701->desc->pwr_timing->after_reset);
	}

	if (st7701->desc->do_sw_reset) {
		mipi_dsi_dcs_soft_reset_multi(&ctx);
		msleep(st7701->desc->pwr_timing->after_reset);
	}

	if (st7701->desc->init_sequence) {
		int ret = st7701->desc->init_sequence(st7701->dsi);
		if (ret)
			return ret;
	}

	mipi_dsi_dcs_exit_sleep_mode_multi(&ctx);
	msleep(st7701->desc->pwr_timing->slpout);

	return ctx.accum_err;
}

static int st7701_enable(struct drm_panel *panel)
{
	struct mipi_dsi_multi_context ctx = { .dsi = to_mipi_dsi_device(panel->dev) };
	mipi_dsi_dcs_set_display_on_multi(&ctx);
	return ctx.accum_err;
}

static int st7701_disable(struct drm_panel *panel)
{
	struct mipi_dsi_multi_context ctx = { .dsi = to_mipi_dsi_device(panel->dev) };
	mipi_dsi_dcs_set_display_off_multi(&ctx);
	return ctx.accum_err;
}

static int st7701_unprepare(struct drm_panel *panel)
{
	struct st7701 *st7701 = to_st7701(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = st7701->dsi };

	mipi_dsi_dcs_enter_sleep_mode_multi(&ctx);
	if (st7701->reset)
		gpiod_set_value_cansleep(st7701->reset, 0);

	return ctx.accum_err;
}

static int st7701_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct st7701 *st7701 = to_st7701(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, st7701->desc->mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	drm_connector_set_orientation_from_panel(connector, panel);
	return 1;
}

static enum drm_panel_orientation st7701_get_orientation(struct drm_panel *panel)
{
	return to_st7701(panel)->orientation;
}

static const struct drm_panel_funcs st7701_funcs = {
	.prepare = st7701_prepare,
	.enable = st7701_enable,
	.disable = st7701_disable,
	.unprepare = st7701_unprepare,
	.get_modes = st7701_get_modes,
	.get_orientation = st7701_get_orientation,
};

static const struct drm_display_mode st7701_mode = {
	.clock = 20000,

	.hdisplay = 480,
	.hsync_start = 480 + 40,
	.hsync_end = 480 + 40 + 20,
	.htotal = 480 + 40 + 20 + 40,

	.vdisplay = 480,
	.vsync_start = 480 + 30,
	.vsync_end = 480 + 30 + 20,
	.vtotal = 480 + 30 + 20 + 30,

	.width_mm = 53,
	.height_mm = 53,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct power_on_timing st7701_pwr_timing = {
	.post_reset = 20,
	.reset_low = 20,
	.after_reset = 120,
	.slpout = 120,
};

static const struct st7701_desc st7701_desc = {
	.mode = &st7701_mode,
	.lanes = 2,
	.flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM,
	.format = MIPI_DSI_FMT_RGB888,
	.init_sequence = st7701_480x480_init_sequence,
	.pwr_timing = &st7701_pwr_timing,
	.do_sw_reset = true,
};

static int st7701_probe(struct mipi_dsi_device *dsi)
{
	struct st7701 *st7701;
	const struct st7701_desc *desc;
	int ret;

	st7701 = devm_kzalloc(&dsi->dev, sizeof(*st7701), GFP_KERNEL);
	if (!st7701)
		return -ENOMEM;

	desc = of_device_get_match_data(&dsi->dev);
	dsi->mode_flags = desc->flags;
	dsi->format = desc->format;
	dsi->lanes = desc->lanes;

	st7701->panel.prepare_prev_first = true;
	st7701->reset = devm_gpiod_get_optional(&dsi->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(st7701->reset)) {
		dev_err(&dsi->dev, "Failed to get reset GPIO\n");
		return PTR_ERR(st7701->reset);
	}

	ret = of_drm_get_panel_orientation(dsi->dev.of_node, &st7701->orientation);
	if (ret < 0)
		st7701->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;

	drm_panel_init(&st7701->panel, &dsi->dev, &st7701_funcs, DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&st7701->panel);
	if (ret)
		return ret;

	drm_panel_add(&st7701->panel);

	mipi_dsi_set_drvdata(dsi, st7701);
	st7701->dsi = dsi;
	st7701->desc = desc;

	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&st7701->panel);

	return ret;
}

static void st7701_remove(struct mipi_dsi_device *dsi)
{
	struct st7701 *st7701 = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&st7701->panel);
}

static const struct of_device_id st7701_of_match[] = {
	{ .compatible = "sitronix,st7701-480x480", .data = &st7701_desc },
	{ }
};
MODULE_DEVICE_TABLE(of, st7701_of_match);

static struct mipi_dsi_driver st7701_driver = {
	.probe = st7701_probe,
	.remove = st7701_remove,
	.driver = {
		.name = "panel-st7701-480x480",
		.of_match_table = st7701_of_match,
	},
};
module_mipi_dsi_driver(st7701_driver);

MODULE_AUTHOR("Adapted from CNflysky");
MODULE_DESCRIPTION("ST7701 480x480 2-Lane MIPI-DSI Panel Driver");
MODULE_LICENSE("GPL");
