/* global $, window */

var FlirCam = function () {
    var _self = this;

    _self.waiters = {};
    _self.repeaters = {};
    _self.cfg = null;

    _self.apiGet = function (url, cb) {
        $.getJSON(url, function (d) {
            if (cb) cb(d);
        });
    };

    _self.apiCmd = function (line) {
        $.getJSON("/api/cmd?line=" + encodeURIComponent(line));
    };

    _self.loadConfig = function () {
        _self.apiGet("/api/config", function (cfg) {
            _self.cfg = cfg;
            _self.applyConfigToUI();
        });
    };

    _self.applyConfigToUI = function () {
        var c = _self.cfg;
        if (!c) return;

        $("#bg_mode").val(c.background);

        $("#thermal_enabled").prop("checked", c.thermal.enabled);
        $("#thermal_smooth").val(c.thermal.smooth);
        $("#thermal_smooth_val").text(c.thermal.smooth);

        $("#th_offx").val(c.thermal.offset_x);
        $("#th_offy").val(c.thermal.offset_y);
        $("#th_scale").val(c.thermal.scale);
        $("#th_opacity").val(c.thermal.opacity);
        $("#th_rot").val(c.thermal.rotate);

        $("#cam_enabled").prop("checked", c.usb_cam.enabled);
        $("#cam_emboss").prop("checked", c.usb_cam.emboss);

        $("#cam_offx").val(c.usb_cam.offset_x);
        $("#cam_offy").val(c.usb_cam.offset_y);
        $("#cam_scale").val(c.usb_cam.scale);
        $("#cam_opacity").val(c.usb_cam.opacity);
        $("#cam_rot").val(c.usb_cam.rotate);

        $(".slider input").each(function () {
            var id = this.id + "_val";
            $("#" + id).text($(this).val());
        });
    };

    _self.handleEvents = function () {

        $("body").on("input", ".slider input", function () {
            $("#" + this.id + "_val").text(this.value);
        });

        $("body").on("change mouseup touchend", ".slider input", function () {
            var id = this.id;
            var v = this.value;

            var map = {
                th_offx:   { src: "thermal", key: "offset_x" },
                th_offy:   { src: "thermal", key: "offset_y" },
                th_scale:  { src: "thermal", key: "scale" },
                th_opacity:{ src: "thermal", key: "opacity" },
                th_rot:    { src: "thermal", key: "rotate_deg" },

                cam_offx:   { src: "usb", key: "offset_x" },
                cam_offy:   { src: "usb", key: "offset_y" },
                cam_scale:  { src: "usb", key: "scale" },
                cam_opacity:{ src: "usb", key: "opacity" },
                cam_rot:    { src: "usb", key: "rotate_deg" }
            };

            if (!map[id]) return;

            _self.apiCmd("set " + map[id].src + " " + map[id].key + " " + v);
        });

        $("body").on("change", "#bg_mode", function () {
            _self.apiCmd("bg " + this.value);
        });

        $("body").on("change", "#thermal_smooth", function () {
            _self.apiCmd("set thermal smooth " + this.value);
        });

        $("body").on("change", "#cam_emboss", function () {
            _self.apiCmd("set usb emboss " + (this.checked ? "1" : "0"));
        });

        $("body").on("click", "#btn_reload", function () {
            $("#stream").attr("src", "/mjpeg?ts=" + Date.now());
        });
    };

    _self.init = function () {
        _self.handleEvents();
        _self.loadConfig();
        console.log("FlirCam initialized");
    };
};
