SECTIONS
{
    .text :
    {
        __gi_start = .;
        KEEP(*(SORT(.gi_fn*)))
        __gi_end = .;
    }
}
