shader_body
{
    //************************************************************
    // NOTE: the body of this shader will be replaced by MilkDrop
    //       whenever a pre-MilkDrop-2 preset is loaded!
    //************************************************************

    // Sample previous frame.
    ret = tex2D(sampler_main, uv).xyz;

    // Darken over time.
    ret -= 0.004;
}
