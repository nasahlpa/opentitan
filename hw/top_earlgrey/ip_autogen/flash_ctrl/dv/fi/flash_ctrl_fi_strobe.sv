module flash_ctrl_fi_strobe;

initial
begin
  $fsdbDumpvars;
end


  integer cmp;

  initial
    begin
			@(posedge tb.dut.rst_ni)
      $fs_inject;
    end

    always@(negedge tb.dut.clk_i) begin
          //change cmp == to case statement.
          //fs_compare either instance or list of signal.
          //maybe list of signals + instance -> check this
          cmp = $fs_compare(tb.dut.mem_tl_o);
          if (1 == cmp)
            $fs_set_status("ON", "tb.dut.mem_tl_o");
          else if (2 == cmp)
            // x or z difference.
            // if we get a lot of x/z - investigate it. initialize memories, flops.
            $fs_set_status("PN", "tb.dut.mem_tl_o");
    end
  
    string status;
    always@(negedge tb.dut.clk_i) begin
          #2;
          cmp = $fs_compare(tb.dut.alert_srcs, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr);
          status = $fs_get_status();
          if (1 == cmp)
            begin
              if (status == "ON")
                $fs_drop_status("OD", "tb.dut.fatal_std_er, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr");
              else if (status == "PN")
                $fs_drop_status("PD", "tb.dut.fatal_std_er, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr");
              else
                $fs_set_status("ND", "tb.dut.fatal_std_er, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr");
            end
          else if (2 == cmp)
            begin
              if (status == "ON")
                $fs_drop_status("OP", "tb.dut.fatal_std_er, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr");
              else if (status == "PN")
                $fs_drop_status("PP", "tb.dut.fatal_std_er, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr");
              else
                $fs_set_status("NP", "tb.dut.fatal_std_er, tb.dut.fi_test_intg_err_q, tb.dut.flash_host_rderr");
            end
        end
endmodule
